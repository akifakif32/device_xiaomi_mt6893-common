/*
 * SPDX-FileCopyrightText: The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

/* libOpenCL.so SHIM: tum cl* cagrilari gercek libOpenCL'e (libOpenCL_real.so) argumanlar bozulmadan yonlendirilir (asm trampolin).
 * clCreateProgramWithBinary'de Mali CLCC rev 0x27 binary'leri bellekte 0x2c'ye donusturulur (conv.h), digerlerine dokunulmaz.
 * Donusum basarisizsa binary AYNEN iletilir (fail-safe). Log: logcat etiketi CLSHIM. */
typedef unsigned long size_t_; typedef unsigned long size_t;
extern void *dlopen(const char *, int);
extern void *dlsym(void *, const char *);
extern int __android_log_write(int, const char *, const char *);
extern int vsnprintf(char *, size_t_, const char *, __builtin_va_list);
extern int getpid(void);
extern int usleep(unsigned);
extern void *malloc(size_t_);
extern void free(void *);
extern void *memcpy(void *, const void *, size_t_);
#include "conv.h"
#define N 140
__attribute__((visibility("hidden"))) void *fwd_tab[N];
static const char *names[N] = {"_Z15khrIcdVendorAddPKc","_Z16initCLTraceLevelv","_Z16khrIcdInitializev","_Z19khrIcdOsLibraryLoadPKc","_Z21khrIcdOsLibraryUnloadPv","_Z24khrIcdOsVendorsEnumeratev","_Z27clSetProgramReleaseCallbackP11_cl_programPFvS0_PvES1_","_Z28khrIcdOsVendorsEnumerateOncev","_Z33khrIcdOsLibraryGetFunctionAddressPvPKc","_Z34clSetProgramSpecializationConstantP11_cl_programjmPKv","_Z34khrIcdContextPropertiesGetPlatformPKlPP15_cl_platform_id","_Z7TraceCLPKciz","clBuildProgram","clCloneKernel","clCompileProgram","clCreateBuffer","clCreateCommandQueue","clCreateCommandQueueWithProperties","clCreateContext","clCreateContextFromType","clCreateEventFromEGLSyncKHR","clCreateEventFromGLsyncKHR","clCreateFromEGLImageKHR","clCreateFromGLBuffer","clCreateFromGLRenderbuffer","clCreateFromGLTexture","clCreateFromGLTexture2D","clCreateFromGLTexture3D","clCreateImage","clCreateImage2D","clCreateImage3D","clCreateKernel","clCreateKernelsInProgram","clCreatePipe","clCreateProgramWithBinary","clCreateProgramWithBuiltInKernels","clCreateProgramWithIL","clCreateProgramWithSource","clCreateSampler","clCreateSamplerWithProperties","clCreateSubBuffer","clCreateSubDevices","clCreateSubDevicesEXT","clCreateUserEvent","clEnqueueAcquireEGLObjectsKHR","clEnqueueAcquireGLObjects","clEnqueueBarrier","clEnqueueBarrierWithWaitList","clEnqueueCopyBuffer","clEnqueueCopyBufferRect","clEnqueueCopyBufferToImage","clEnqueueCopyImage","clEnqueueCopyImageToBuffer","clEnqueueFillBuffer","clEnqueueFillImage","clEnqueueMapBuffer","clEnqueueMapImage","clEnqueueMarker","clEnqueueMarkerWithWaitList","clEnqueueMigrateMemObjects","clEnqueueNDRangeKernel","clEnqueueNativeKernel","clEnqueueReadBuffer","clEnqueueReadBufferRect","clEnqueueReadImage","clEnqueueReleaseEGLObjectsKHR","clEnqueueReleaseGLObjects","clEnqueueSVMFree","clEnqueueSVMMap","clEnqueueSVMMemFill","clEnqueueSVMMemcpy","clEnqueueSVMMigrateMem","clEnqueueSVMUnmap","clEnqueueTask","clEnqueueUnmapMemObject","clEnqueueWaitForEvents","clEnqueueWriteBuffer","clEnqueueWriteBufferRect","clEnqueueWriteImage","clFinish","clFlush","clGetCommandQueueInfo","clGetContextInfo","clGetDeviceAndHostTimer","clGetDeviceIDs","clGetDeviceInfo","clGetEventInfo","clGetEventProfilingInfo","clGetExtensionFunctionAddress","clGetExtensionFunctionAddressForPlatform","clGetGLContextInfoKHR","clGetGLObjectInfo","clGetGLTextureInfo","clGetHostTimer","clGetImageInfo","clGetKernelArgInfo","clGetKernelInfo","clGetKernelSubGroupInfo","clGetKernelSubGroupInfoKHR","clGetKernelWorkGroupInfo","clGetMemObjectInfo","clGetPipeInfo","clGetPlatformIDs","clGetPlatformInfo","clGetProgramBuildInfo","clGetProgramInfo","clGetSamplerInfo","clGetSupportedImageFormats","clLinkProgram","clReleaseCommandQueue","clReleaseContext","clReleaseDevice","clReleaseDeviceEXT","clReleaseEvent","clReleaseKernel","clReleaseMemObject","clReleaseProgram","clReleaseSampler","clRetainCommandQueue","clRetainContext","clRetainDevice","clRetainDeviceEXT","clRetainEvent","clRetainKernel","clRetainMemObject","clRetainProgram","clRetainSampler","clSVMAlloc","clSVMFree","clSetCommandQueueProperty","clSetDefaultDeviceCommandQueue","clSetEventCallback","clSetKernelArg","clSetKernelArgSVMPointer","clSetKernelExecInfo","clSetMemObjectDestructorCallback","clSetUserEventStatus","clUnloadCompiler","clUnloadPlatformCompiler","clWaitForEvents"};
#define I_CPWB 34
#define I_CLENQUEUENDRANGEKERNEL 60
#define I_CLFINISH 79
#define I_CLGETEVENTINFO 86
#define I_CLRELEASEEVENT 113
#define I_CLGETKERNELINFO 96
#define I_CLCREATEBUFFER 15
#define I_CLCREATESUBBUFFER 40
#define I_CLRELEASEMEMOBJECT 115
#define I_CLGETMEMOBJECTINFO 100
#define I_CK 31
static void L(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
static void L(const char *fmt, ...) {
    char b[512]; __builtin_va_list ap; __builtin_va_start(ap, fmt); vsnprintf(b, sizeof b, fmt, ap); __builtin_va_end(ap);
    __android_log_write(4, "CLSHIM", b);
}
typedef void *(*cpwb_t)(void *, unsigned, void *const *, const size_t_ *, const unsigned char **, int *, int *);
typedef void *(*ck_t)(void *, const char *, int *);
static cpwb_t real_cpwb; static ck_t real_ck;
/* ---- v23: LLVM'siz yeniden insa + opcode'a gore kernel cikarma ---- */
#define MAXK 600
typedef struct { size_t mo, ml; int hasm; size_t ko; unsigned nof; size_t nl; int del; } kr_t;

static unsigned char *rebuild(const unsigned char *in, size_t n, size_t *outn, int *nrem, int *ntot) {
    static kr_t kr[MAXK]; int nk = 0; size_t strt = 0, strtl = 0; size_t pm = 0, pml = 0; int havem = 0;
    if (n < 0x20 || !tagis(in, "CLCC")) return 0;
    size_t r = 0x14;
    while (r + 8 <= n) { size_t L = rd32(in + r + 4); if (r + 8 + L > n) return 0;
        if (tagis(in + r, "LLVM")) { }
        else if (tagis(in + r, "MBSX")) { pm = r; pml = L; havem = 1; }
        else if (tagis(in + r, "KRNL")) { if (nk >= MAXK || L != 20) return 0; kr[nk].mo = pm; kr[nk].ml = pml; kr[nk].hasm = havem; kr[nk].ko = r; kr[nk].del = 0; nk++; havem = 0; }
        else if (tagis(in + r, "STRT")) { strt = r; strtl = L; }
        else return 0;
        r += 8 + L; }
    if (r != n || !strt || strtl < 16) return 0;
    const unsigned char *sp = in + strt + 8; size_t so = 15; int ni = 0;
    if (sp[0] != '-') return 0;
    while (so < strtl && ni < nk) { size_t e = so; while (e < strtl && sp[e]) e++; kr[ni].nof = (unsigned)so; kr[ni].nl = e - so; ni++; so = e + 1; }
    if (ni != nk) return 0;
    int rem = 0;
    for (int i = 0; i < nk; i++) { if (!kr[i].hasm) { kr[i].del = 1; rem++; } }
    *nrem = rem; *ntot = nk;
    unsigned char *o = (unsigned char *)malloc(n + 64); if (!o) return 0;
    memcpy(o, in, 0x14); size_t w = 0x14; int kept = 0; unsigned noff = 15;
    unsigned char *sbuf = (unsigned char *)malloc(strtl + 16); if (!sbuf) { free(o); return 0; }
    memcpy(sbuf, sp, 15); size_t sw = 15;
    for (int i = 0; i < nk; i++) { if (kr[i].del) continue;
        if (kr[i].hasm) { memcpy(o + w, in + kr[i].mo, 8 + kr[i].ml); w += 8 + kr[i].ml; }
        memcpy(o + w, in + kr[i].ko, 28);
        wr32(o + w + 8 + 8, noff); wr32(o + w + 8 + 12, (unsigned)(2 * kept + 2));
        w += 28;
        memcpy(sbuf + sw, sp + kr[i].nof, kr[i].nl); sbuf[sw + kr[i].nl] = 0; sw += kr[i].nl + 1; noff += (unsigned)(kr[i].nl + 1); kept++; }
    memcpy(o + w, "STRT", 4); wr32(o + w + 4, (unsigned)sw); memcpy(o + w + 8, sbuf, sw); w += 8 + sw; free(sbuf);
    wr32(o + 8, (unsigned)(2 * kept + 2));
    *outn = w; return o;
}
static void *hook_cpwb(void *ctx, unsigned n, void *const *devs, const size_t_ *lens, const unsigned char **bins, int *st, int *err) {
    if (!lens || !bins || n == 0 || n > 16) return real_cpwb(ctx, n, devs, lens, bins, st, err);
    const unsigned char *nb[16]; size_t_ nl[16]; unsigned char *fr[16]; unsigned i; int nconv = 0;
    for (i = 0; i < n; i++) {
        nb[i] = bins[i]; nl[i] = lens[i]; fr[i] = 0;
        if (bins[i] && lens[i] > 0x2c) {
            size_t_ ol = 0; unsigned char *c = clcc_convert(bins[i], lens[i], &ol);
            if (c) { nb[i] = c; nl[i] = ol; fr[i] = c; nconv++; }
        }
    }
    void *p = real_cpwb(ctx, n, devs, nl, nb, st, err);
    if (!p && err && *err == -42 && n == 1 && nb[0]) {
        /* HDR (libxmi) program: LLVM'li yapi r49p1'de gecersiz. LLVM'siz, yerel numaralamayla yeniden insa et. */
        size_t vl = 0; int nrem = 0, ntot = 0; unsigned char *v = rebuild(nb[0], nl[0], &vl, &nrem, &ntot);
        if (v) {
            const unsigned char *vb[1] = { v }; size_t vls[1] = { vl }; int e2 = 0;
            void *p2 = real_cpwb(ctx, 1, devs, vls, vb, st, &e2);
            free(v);
            if (p2 && e2 == 0) { p = p2; *err = 0; L("REBUILD ok removed=%d/%d", nrem, ntot); }
            else L("REBUILD failed rc=%d", e2);
        }
    }
    for (i = 0; i < n; i++) if (fr[i]) free(fr[i]);
    if (err && *err != 0) L("PROG failed n=%u len0=%lu conv=%d rc=%d", n, (unsigned long)lens[0], nconv, *err);
    return p;
}

typedef int (*enq_t)(void *, void *, unsigned, const size_t_ *, const size_t_ *, const size_t_ *, unsigned, const void *, void **);
typedef int (*fin_t)(void *);
typedef int (*gei_t)(void *, unsigned, size_t_, void *, size_t_ *);
typedef int (*rel_t)(void *);
typedef int (*gki_t)(void *, unsigned, size_t_, void *, size_t_ *);
static enq_t real_enq; static fin_t real_fin; static gei_t real_gei; static rel_t real_rel; static gki_t real_gki;
static unsigned long nenq;
static int hook_enq(void *q, void *k, unsigned dim, const size_t_ *go, const size_t_ *gws, const size_t_ *lws, unsigned ne, const void *ewl, void **ev) {
    void *own = 0; void **evp = ev ? ev : &own;
    int r = real_enq(q, k, dim, go, gws, lws, ne, ewl, evp);
    char nm[96]; nm[0] = 0; real_gki(k, 0x1190, sizeof nm, nm, 0);
    unsigned long g0 = gws ? gws[0] : 0, g1 = gws && dim > 1 ? gws[1] : 1, g2 = gws && dim > 2 ? gws[2] : 1;
    nenq++;
    if (r != 0) { L("ENQFAIL pid=%d #%lu %.60s dim=%u gws=%lu,%lu,%lu rc=%d", getpid(), nenq, nm, dim, g0, g1, g2, r); if (!ev && own) real_rel(own); return r; }
    real_fin(q);
    int st = 0; real_gei(*evp, 0x11D3, sizeof st, &st, 0);
    if (st < 0) L("KERNFAIL pid=%d #%lu %.60s dim=%u gws=%lu,%lu,%lu status=%d", getpid(), nenq, nm, dim, g0, g1, g2, st);
    if (!ev && own) real_rel(own);
    return r;
}


typedef void *(*cb_t)(void *, unsigned long, size_t_, void *, int *);
typedef void *(*csb_t)(void *, unsigned long, unsigned, const void *, int *);
typedef int (*rmo_t)(void *);
typedef int (*gmi_t)(void *, unsigned, size_t_, void *, size_t_ *);
static cb_t real_cb; static csb_t real_csb; static rmo_t real_rmo; static gmi_t real_gmi;
#define PADBYTES (128u * 1024u)
#define MAXMAP 32768
static void *mv[MAXMAP], *mp[MAXMAP]; static int nm; static volatile int mlock; static unsigned long nwrap, nfall;
static void lk(void) { while (__sync_lock_test_and_set(&mlock, 1)) { } }
static void ul(void) { __sync_lock_release(&mlock); }
static void map_add(void *v, void *p) { lk(); for (int i = 0; i < nm; i++) if (mv[i] == v) { mp[i] = p; ul(); return; } if (nm < MAXMAP) { mv[nm] = v; mp[nm] = p; nm++; } ul(); }
static void *map_find(void *v) { void *r = 0; lk(); for (int i = 0; i < nm; i++) if (mv[i] == v) { r = mp[i]; break; } ul(); return r; }
static void map_del(void *v) { lk(); for (int i = 0; i < nm; i++) if (mv[i] == v) { mv[i] = mv[nm - 1]; mp[i] = mp[nm - 1]; nm--; break; } ul(); }
/* Her (host pointer kullanmayan) tampon, iki tarafi PADBYTES bosluklu bir ebeveynin ORTASINDAKI alt-tampon olarak sunulur:
 * ct_descriptor_7x7 gibi kernel'ler tampon sinirinin disina okuyor; r32p1 sessiz geciyor, r49p1 GPU sayfa hatasi veriyor. */
static void *hook_cb(void *ctx, unsigned long flags, size_t_ size, void *host, int *err) {
    if ((flags & 0x8UL) || size == 0 || (flags & 0x20UL && !host)) return real_cb(ctx, flags, size, host, err);
    size_t_ total = size + 2 * (size_t_)PADBYTES; unsigned char *tmp = 0; void *hp = 0;
    if (flags & 0x20UL) {
        tmp = (unsigned char *)malloc(total);
        if (!tmp) return real_cb(ctx, flags, size, host, err);
        for (size_t_ i = 0; i < PADBYTES; i++) tmp[i] = 0;
        for (size_t_ i = 0; i < PADBYTES; i++) tmp[PADBYTES + size + i] = 0;
        memcpy(tmp + PADBYTES, host, size);
        hp = tmp;
    }
    int e = 0; void *par = real_cb(ctx, flags, total, hp, &e);
    if (tmp) free(tmp);
    if (!par || e) { if (par) real_rmo(par); nfall++; return real_cb(ctx, flags, size, host, err); }
    size_t_ reg[2] = { PADBYTES, size }; void *sub = real_csb(par, 0, 0x1220, reg, &e);
    if (!sub || e) { real_rmo(par); nfall++; return real_cb(ctx, flags, size, host, err); }
    real_rmo(par);                         /* ebeveyn, alt-tampon tarafindan tutuluyor */
    map_add(sub, par); nwrap++; if (nwrap == 1) L("PAD aktif pid=%d (iki tarafli %u bayt dolgu, alt-tampon)", getpid(), PADBYTES);
    if (err) {
        *err = 0;
    }
    return sub;
}
static void *hook_csb(void *buffer, unsigned long flags, unsigned type, const void *info, int *err) {
    void *par = (type == 0x1220 && info) ? map_find(buffer) : 0;
    if (par) { size_t_ reg[2]; memcpy(reg, info, sizeof reg); reg[0] += PADBYTES; return real_csb(par, flags, type, reg, err); }
    return real_csb(buffer, flags, type, info, err);
}
static int hook_rmo(void *mem) {
    if (mem) {
        unsigned rc = 0;
        if (map_find(mem) && real_gmi(mem, 0x1105, sizeof rc, &rc, 0) == 0 && rc <= 1) map_del(mem);
    }
    return real_rmo(mem);
}
static void *hook_ck(void *prog, const char *name, int *err) {
    void *k = real_ck(prog, name, err);
    if (err && *err != 0) L("KFAIL pid=%d name=%.60s rc=%d", getpid(), name ? name : "?", *err);
    return k;
}
__attribute__((constructor)) static void init(void) {
    void *h = dlopen("/vendor/lib64/libOpenCL_real.so", 2);
    if (!h) { L("INIT dlopen real BASARISIZ"); return; }
    for (int i = 0; i < N; i++) fwd_tab[i] = dlsym(h, names[i]);
    real_cpwb = (cpwb_t)fwd_tab[I_CPWB]; real_ck = (ck_t)fwd_tab[I_CK];
    real_enq = (enq_t)fwd_tab[I_CLENQUEUENDRANGEKERNEL]; real_fin = (fin_t)fwd_tab[I_CLFINISH]; real_gei = (gei_t)fwd_tab[I_CLGETEVENTINFO]; real_rel = (rel_t)fwd_tab[I_CLRELEASEEVENT]; real_gki = (gki_t)fwd_tab[I_CLGETKERNELINFO]; real_cb = (cb_t)fwd_tab[I_CLCREATEBUFFER]; real_csb = (csb_t)fwd_tab[I_CLCREATESUBBUFFER]; real_rmo = (rmo_t)fwd_tab[I_CLRELEASEMEMOBJECT]; real_gmi = (gmi_t)fwd_tab[I_CLGETMEMOBJECTINFO];
    fwd_tab[I_CPWB] = (void *)hook_cpwb; fwd_tab[I_CK] = (void *)hook_ck; fwd_tab[I_CLENQUEUENDRANGEKERNEL] = (void *)hook_enq; fwd_tab[I_CLCREATEBUFFER] = (void *)hook_cb; fwd_tab[I_CLCREATESUBBUFFER] = (void *)hook_csb; fwd_tab[I_CLRELEASEMEMOBJECT] = (void *)hook_rmo;

}
