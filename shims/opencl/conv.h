/*
 * SPDX-FileCopyrightText: The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

/* conv.h - Mali CLCC binary rev 0x27 -> 0x2c donusturucu (test: transpose/conv_direct_transpose_ocl cikti-esit, 166 kernel yaratma dogrulandi).
 * clcc_convert(): girdi 0x27 CLCC ise malloc'lu yeni tampon dondurur, degilse/uyumsuzsa NULL (girdiye dokunulmaz). */
static unsigned rd32(const unsigned char *p) { return p[0] | (p[1] << 8) | (p[2] << 16) | ((unsigned)p[3] << 24); }
static void wr32(unsigned char *p, unsigned v) { p[0] = v; p[1] = v >> 8; p[2] = v >> 16; p[3] = v >> 24; }
static int tagis(const unsigned char *p, const char *t) { return p[0] == (unsigned char)t[0] && p[1] == (unsigned char)t[1] && p[2] == (unsigned char)t[2] && p[3] == (unsigned char)t[3]; }
/* tek MBSX blogu: in[0..len) -> out. donus: yazilan bayt, 0=donusturulmedi (rev != 0x27), -1=hata.
 * EBIN icindeki RLOC listesi silinir; idx==0xFFFF olanlar (sabitler) FCST kayitlarina cevrilip EBIN oncesine eklenir. */
static long cv_mbsx(const unsigned char *in, size_t len, unsigned char *out) {
    if (len < 60 || !tagis(in, "MBSX") || !tagis(in + 12, "MBS2") || !tagis(in + 24, "VEHW") || !tagis(in + 44, "CCOM") || !tagis(in + 52, "CMMN")) return -1;
    if (rd32(in + 20) != 0x27) return 0;
    size_t cm_lo = 60, cm_hi = 60 + rd32(in + 56);
    if (cm_hi > len) return -1;
    size_t e = 0; for (size_t i = cm_lo + 4; i + 8 <= cm_hi; i++) if (tagis(in + i, "EBIN") && rd32(in + i - 4) == 1) { e = i; break; }
    if (!e) return -1;
    size_t plo = e + 8; if (plo + 16 > len) return -1;
    unsigned n = rd32(in + plo + 8), z = rd32(in + plo + 12), el = rd32(in + e + 4);
    if (z != 0 || n > 4096) return -1;
    size_t cut = 8 + 16 * (size_t)n;
    if (el < 16 + 16 * (size_t)n || plo + 8 + cut > len) return -1;
    unsigned nf = 0;
    for (unsigned i = 0; i < n; i++) {
        const unsigned char *r = in + plo + 16 + 16 * (size_t)i;
        if (!tagis(r, "RLOC") || rd32(r + 4) != 8) return -1;
        if ((r[8] | (r[9] << 8)) == 0xFFFF) nf++;
    }
    size_t o = 0;
    memcpy(out + o, in, e - 4); o += e - 4;
    wr32(out + o, nf); o += 4;                                    /* nFCST */
    for (unsigned i = 0; i < n; i++) {
        const unsigned char *r = in + plo + 16 + 16 * (size_t)i;
        if ((r[8] | (r[9] << 8)) != 0xFFFF) continue;
        out[o++] = 'F'; out[o++] = 'C'; out[o++] = 'S'; out[o++] = 'T'; wr32(out + o, 8); o += 4;
        out[o++] = 4; out[o++] = 0; out[o++] = r[10]; out[o++] = r[11];   /* kind=4 (32 bit), ofset */
        { unsigned v = rd32(r + 12);
          /* v5: texture/sampler tutamaci (kaynak tablosu 1->2): 0x180N1000 -> 0x280N2000 (N=0..7); native 0x2c'de 28002000/28012000/28022000 */
          /* v6: sampler numarasi da alt yarida (0x1000|sampler 0..7): 0x18001002 gibi tutamaclar da cevrilir */
          if ((v & 0xFFF8FFF8u) == 0x18001000u) v += 0x10001000u;
          wr32(out + o, v); o += 4; }                                              /* deger (lo,hi) */
    }
    memcpy(out + o, in + e - 4, plo + 8 - (e - 4)); o += plo + 8 - (e - 4);
    memcpy(out + o, in + plo + 8 + cut, len - (plo + 8 + cut)); o += len - (plo + 8 + cut);
    long delta = (long)(o) - (long)len;
    wr32(out + 4, rd32(out + 4) + delta);            /* MBSX */
    wr32(out + 16, rd32(out + 16) + delta);          /* MBS2 */
    wr32(out + 48, rd32(out + 48) + delta);          /* CCOM */
    wr32(out + 56, rd32(out + 56) + delta);          /* CMMN */
    wr32(out + e + 4 + 4 + 16 * (size_t)nf, el - cut);   /* EBIN (nFCST alani + FCST'ler kadar kaymis) */
    out[20] = 0x2c; out[21] = out[22] = out[23] = 0;
    /* v3: goruntu (TPSA) sembollerinde ilk u32 0x00000004 -> 0xFF000004 (0x2c boyle) */
    for (size_t i = 60; i + 16 <= o; i++) {
        if (!tagis(out + i, "SYMB")) continue;
        size_t sl = rd32(out + i + 4);
        if (sl < 16 || i + 8 + sl > o || !tagis(out + i + 8, "STRI")) continue;
        size_t nl = rd32(out + i + 12), a = i + 16 + nl;
        if (a + 8 > i + 8 + sl) continue;
        int has = 0; for (size_t j = a; j + 4 <= i + 8 + sl; j++) if (tagis(out + j, "TPSA")) { has = 1; break; }
        if (has && rd32(out + a) == 4) wr32(out + a, 0xFF000004u);
    }
    /* v4: image-YAZMA komutu (b1=0xc0,b4=0x26,b6=0x67): 3. bayt 0x04/0x14/0x24 -> bit1 set (0x2c'de hep 0x06) */
    { size_t eb = 0; for (size_t i = 60; i + 8 <= o; i++) if (tagis(out + i, "EBIN")) eb = i;
      if (eb) { size_t el = rd32(out + eb + 4), end = eb + 8 + el;
        if (end <= o) for (size_t j = eb + 8; j + 8 <= end; j++) if (tagis(out + j, "OBJC")) {
            size_t ol = rd32(out + j + 4);
            if (j + 8 + ol == end) { for (size_t k = j + 8; k + 8 <= end; k += 8) { unsigned char *x = out + k;
                if (x[1] == 0xc0 && x[4] == 0x26 && x[6] == 0x67 && (x[2] == 0x04 || x[2] == 0x14 || x[2] == 0x24)) x[2] |= 0x02; } break; } } } }
    return (long)o;
}
static unsigned char *clcc_convert(const unsigned char *in, size_t len, size_t *outlen) {
    if (len < 0x20 || !tagis(in, "CLCC")) return 0;
    size_t o = 0x14, nb = 0; int any = 0;
    while (o + 8 <= len) { size_t L = rd32(in + o + 4); if (o + 8 + L > len) return 0; if (tagis(in + o, "MBSX")) nb++; o += 8 + L; }
    if (o != len || !nb) return 0;
    unsigned char *res = (unsigned char *)malloc(len + 4 * nb + 64 + 16 * (len / 16)); if (!res) return 0;
    memcpy(res, in, 0x14); size_t w = 0x14; o = 0x14;
    while (o + 8 <= len) {
        size_t L = rd32(in + o + 4), bl = 8 + L;
        if (tagis(in + o, "MBSX")) {
            long r = cv_mbsx(in + o, bl, res + w);
            if (r < 0) { free(res); return 0; }
            if (r == 0) { memcpy(res + w, in + o, bl); w += bl; } else { w += (size_t)r; any = 1; }
        } else { memcpy(res + w, in + o, bl); w += bl; }
        o += bl;
    }
    if (!any) { free(res); return 0; }
    *outlen = w; return res;
}
