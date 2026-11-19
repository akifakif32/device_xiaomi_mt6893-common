/*
 * SPDX-FileCopyrightText: The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */
#define LOG_TAG "gpud_shim"

#include <android/hardware_buffer.h>
#include <dlfcn.h>
#include <log/log.h>
#include <pthread.h>
#include <stdint.h>
#include <string.h>

/* Not exported by the stock libged and libgpu_aux, but imported by the r49p1 libGLES_mali. */
void ged_fr_swd_frame_destroy(void) {}
void ged_fr_swd_mark_frame(int type, int frame, void* a, void* b) { (void)type;(void)frame;(void)a;(void)b; }

/*
 * The r49p1 driver calls this for MTK block / compressed video buffers (0x10f,
 * 0x7f000001, 0x7f000002): src is the original buffer, dst is a freshly
 * allocated NV21 buffer. 0 = success. On failure the driver keeps the original
 * buffer (and Chromium/WebView video turns into green noise).
 *
 * Stock A12 libgpu_aux has no GuiExtBlitAHardwareBuffer, but it does the
 * same conversion through MDP in GpuAuxPrepareBuffer(), which returns its own
 * NV21 buffer. Convert with that and copy the result into dst.
 */
typedef void* (*create_ctx_fn)(int format, int count);
typedef int (*prepare_fn)(void* ctx, void* src);
typedef void* (*get_aux_fn)(void* ctx);

static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;
static int g_init;
static void* g_ctx;
static create_ctx_fn p_create;
static prepare_fn p_prepare;
static get_aux_fn p_get_aux;

static int init_locked(void) {
    if (g_init) return g_ctx ? 0 : -1;
    g_init = 1;
    void* h = dlopen("libgpu_aux.so", RTLD_NOW | RTLD_GLOBAL);
    if (!h) { ALOGE("dlopen libgpu_aux.so: %s", dlerror()); return -1; }
    p_create = (create_ctx_fn)dlsym(h, "GpuAuxCreateContext");
    p_prepare = (prepare_fn)dlsym(h, "GpuAuxPrepareBuffer");
    p_get_aux = (get_aux_fn)dlsym(h, "GpuAuxGetCurrentAuxBuffer");
    if (!p_create || !p_prepare || !p_get_aux) { ALOGE("libgpu_aux symbols missing"); return -1; }
    g_ctx = p_create(0x11 /* HAL_PIXEL_FORMAT_YCrCb_420_SP (NV21) */, 2);
    if (!g_ctx) { ALOGE("GpuAuxCreateContext failed"); return -1; }
    return 0;
}

static int copy_nv21(AHardwareBuffer* from, AHardwareBuffer* to) {
    AHardwareBuffer_Desc df, dt;
    AHardwareBuffer_describe(from, &df);
    AHardwareBuffer_describe(to, &dt);
    AHardwareBuffer_Planes pf, pt;
    memset(&pf, 0, sizeof(pf));
    memset(&pt, 0, sizeof(pt));
    if (AHardwareBuffer_lockPlanes(from, AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN, -1, NULL, &pf)) return -1;
    if (AHardwareBuffer_lockPlanes(to, AHARDWAREBUFFER_USAGE_CPU_WRITE_OFTEN, -1, NULL, &pt)) {
        AHardwareBuffer_unlock(from, NULL);
        return -1;
    }
    uint32_t w = df.width < dt.width ? df.width : dt.width;
    uint32_t h = df.height < dt.height ? df.height : dt.height;
    int rc = 0;
    if (pf.planeCount < 2 || pt.planeCount < 2) {
        rc = -1;
    } else {
        for (uint32_t y = 0; y < h; y++)
            memcpy((uint8_t*)pt.planes[0].data + (size_t)y * pt.planes[0].rowStride,
                   (uint8_t*)pf.planes[0].data + (size_t)y * pf.planes[0].rowStride, w);
        for (uint32_t y = 0; y < h / 2; y++)
            memcpy((uint8_t*)pt.planes[1].data + (size_t)y * pt.planes[1].rowStride,
                   (uint8_t*)pf.planes[1].data + (size_t)y * pf.planes[1].rowStride, w);
    }
    AHardwareBuffer_unlock(from, NULL);
    AHardwareBuffer_unlock(to, NULL);
    return rc;
}

int GpuAuxBlitAHardwareBuffer(void* src, void* dst) {
    int rc = -1;
    pthread_mutex_lock(&g_lock);
    if (src && dst && init_locked() == 0 && p_prepare(g_ctx, src) == 0) {
        void* aux = p_get_aux(g_ctx);
        if (aux) rc = copy_nv21((AHardwareBuffer*)aux, (AHardwareBuffer*)dst);
    }
    pthread_mutex_unlock(&g_lock);
    return rc;
}
