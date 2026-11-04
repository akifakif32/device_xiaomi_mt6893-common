/*
 * SPDX-FileCopyrightText: The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

/* Not exported by the stock libged and libgpu_aux, but imported by the r49p1 libGLES_mali. */

void ged_fr_swd_frame_destroy(void) {}

void ged_fr_swd_mark_frame(int type, int frame, void* a, void* b) {
    (void)type;
    (void)frame;
    (void)a;
    (void)b;
}

/* The driver keeps the original buffer when this fails. */
int GpuAuxBlitAHardwareBuffer(void* dst, void* src) {
    (void)dst;
    (void)src;
    return -1;
}
