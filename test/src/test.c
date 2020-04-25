#include "test.h"
#include <stdalign.h>

extern int should_print, should_count, total_tests, num_correct, num_incorrect;

static char BUF[3000] __attribute__ ((aligned (16)));

static void
bench_init (void)
{
  typedef unsigned long long int ull_t;
  size_t i = 0;

  for (; i < 2992; i += 16)
    {
      ull_t rnd1, rnd2;
      RND64 (rnd1);
      RND64 (rnd2);
      _mm_store_si128 ((__m128i *)(BUF + i), _mm_set_epi64x (rnd1, rnd2));
    }
}

void
time_test (void (*func) (), int num_tests, const char *message)
{
  int pid, status;

  // don't want to print/count while testing performance
  // too many tests and the same errors over and over
  STOP_PRINTING
  STOP_COUNTING

  if (!(pid = fork ()))
    {
      volatile clock_t t_i, t_f;
      // warm up CPU+caches so timing is more consistent
      WARMUP (func)

      t_i = clock ();
      for (int i = 0; i < ITERATIONS; ++i)
        func ();
      t_f = clock ();

      double diff = (t_f - t_i) / CLOCKS_PER_SEC;
      printf ("%18s - %f\n", message, diff);
      exit (0);
    }
  waitpid (pid, &status, 0);
  if (status != 0)
    printf ("%15s: (process exited with code: %d)\n", message, status);

  RESUME_COUNTING
  RESUME_PRINTING
}

static void
t_basic_case (char *(*f) (char *restrict s, const char *restrict d),
              const char *name)
{
  char one[50] = "first, second, third";
  char two[50];

  /* Basic test. */
  assert_equal_str ("first", f (one, ", "), "1");
  assert_equal_str ("first", one, "2");
  assert_equal_str ("second", f ((char *)NULL, ", "), "3");
  assert_equal_str ("third", f ((char *)NULL, ", "), "4");
  assert_equal_str (0, f ((char *)NULL, ", "), "5");
  (void)strcpy (one, ", first, ");
  /* Extra delims, 1 tok. */
  assert_equal_str ("first", f (one, ", "), "6");
  assert_equal_str (0, f (0, ", "), "7");

  (void)strcpy (one, "1a, 1b; 2a, 2b");
  /* Changing delim lists. */
  assert_equal_str ("1a", f (one, ", "), "8");
  assert_equal_str ("1b", f ((char *)NULL, "; "), "9");
  assert_equal_str ("2a", f ((char *)NULL, ", "), "10");

  (void)strcpy (two, "x-y");
  /* New string before done. */
  assert_equal_str ("x", f (two, "-"), "11");
  assert_equal_str ("y", f ((char *)NULL, "-"), "12");
  assert_equal_str (0, f ((char *)NULL, "-"), "13");

  (void)strcpy (one, "a,b, c,, ,d");
  /* Different separators. */
  assert_equal_str ("a", f (one, ", "), "14");
  assert_equal_str ("b", f ((char *)NULL, ", "), "15");
  /* Permute list too. */
  assert_equal_str ("c", f ((char *)NULL, " ,"), "16");
  assert_equal_str ("d", f ((char *)NULL, " ,"), "17");
  assert_equal_str (0, f ((char *)NULL, ", "), "18");
  /* Persistence. */
  assert_equal_str (0, f ((char *)NULL, ", "), "19");

  (void)strcpy (one, ", ");
  assert_equal_str (0, f (one, ", "), "20"); /* No tokens. */

  (void)strcpy (one, "");
  assert_equal_str (0, f (one, ", "), "21"); /* Empty string. */

  (void)strcpy (one, "abc");
  assert_equal_str ("abc", f (one, ", "), "22"); /* No delimiters. */
  assert_equal_str (0, f ((char *)NULL, ", "), "23");

  (void)strcpy (one, "abc");
  assert_equal_str ("abc", f (one, ""), "24"); /* Empty delimiter list. */
  assert_equal_str (0, f ((char *)NULL, ""), "25");

  (void)strcpy (one, "abcdefgh");

  (void)strcpy (one, "a,b,c");
  /* Basics again... */
  assert_equal_str ("a", f (one, ","), "26");
  assert_equal_str ("b", f ((char *)NULL, ","), "27");
  assert_equal_str ("c", f ((char *)NULL, ","), "28");
  assert_equal_str (0, f ((char *)NULL, ","), "29");
  /* Stomped past end? */
  assert_equal_str ("gh", one + 6, "30");
  /* Stomped old tokens? */
  assert_equal_str ("a", one, "31");
  assert_equal_str ("b", one + 2, "32");
  assert_equal_str ("c", one + 4, "33");

  char str[80] = "1,2,,3,4";
  const char s[2] = ",";
  assert_equal_str ("1", f (str, s), "f(\"1,2,,3,4\", \",\") first time");
  assert_equal_str ("2", f (NULL, s), "f(\"1,2,,3,4\", \",\") second time");
  assert_equal_str ("3", f (NULL, s), "f(\"1,2,,3,4\", \",\") third time");
  assert_equal_str ("4", f (NULL, s), "f(\"1,2,,3,4\", \",\") fourth time");
  assert_equal_str (NULL, f (NULL, s), "f(\"1,2,,3,4\", \",\") fifth time");

  char str2[80] = ";;;33;;6;;77;";
  const char d[3] = ";;";
  assert_equal_str ("33", f (str2, d),
                    "f(\";;;33;;6;;77;\", \";;\") first time");
  assert_equal_str ("6", f (NULL, d),
                    "f(\";;;33;;6;;77;\", \";;\") second time");
  assert_equal_str ("77", f (NULL, d),
                    "f(\";;;33;;6;;77;\", \";;\") third time");
  assert_equal_str (NULL, f (NULL, d),
                    "f(\";;;33;;6;;77;\", \";;\") fourth time");

  char str3[80] = ";;;33;;6;;77;";
  char str4[80] = ";;;33;;6;;77;";
  assert_equal_str (strtok (str4, ""), f (str3, ""), "NUL delim");
  assert_equal_str (strtok (NULL, ""), f (NULL, ""), "NUL delim");
}

static void
test_basic (void)
{
  t_basic_case (&strok_a, TOKEN (strok_a));
  t_basic_case (&strok_b, TOKEN (strok_b));
  t_basic_case (&strok_naive, TOKEN (strok_naive));
}

static void
_1021_127 (char *(*f) (char *restrict s, const char *restrict d),
           const char *fname)
{
  char s[]
      = "         "
        "09?i:xg7>MOG\"#w:p^l}^%%,E<xj46H*L\\+L{WXTmTCF2*6{mdL,x+w$]= "
        "7$UMt=xw~C7~zU_h)ej1$8QP$ZXCmfGQM "
        "{eECFgP=iqV\"]51R(SwnnXW{5Yw^&i)@FX\4GU.~N(n8Ds[,BS$3 "
        "8z@uj+ZI?lP.0eU .=F+I//YIGZK49{,;klX.{]x'}k=ucuj_#^\r`?\"`OF*dgB "
        "Xd>m5)UbIm4@\\[hri\\IH:'V-hY_(&hI=#&;nN{SvsG2.<\"rW;l^#da+!Q<u1ufas."
        "\"]emH$~\\WwwF^B9m{ "
        "eXGSG.HvL}p_gT1<j.0?]Xi(5nn/@)vWc5z}+hB^,&B6C3U*V:6+m=inI<`'B "
        "r2Nf~Iscp\"a*gvg~odz!Y%%LK$d-\\M "
        "r&Q:ERDJ<J(k?\"3\"-c*P>X%%</"
        "m`S>iq_e$%%ru%%!u@auqcf]%%);6\"<o-DaS8c&M:g#D$O@.C0q6Rzd.x^9p8h42v1&"
        "P#~9Kl--`0tU^8FGbjM/`OE,L|<<qv./>O6\"(+WG0o\\r/"
        "sIn?ec;J'tU?VDO7n{dvw7P>Nq*HNOPUg)K$%%?3{/"
        "FXkF>Yv99WU]'8&dIX:;~O'1)~;O^83d/R_]Mvi:wjo#rgm^l5t$uat "
        "%%6/I%%6XZLlgWT7SL\\g\"\f|7WF9lw<0f<{[H^f/8-[4/"
        "r`%%gMCEd^gGXba8N\"Iv?e.3:t:\7rz3t.%%M\\j/"
        "Sgh\\Z;hEk~0#HO;1rNC92Y@V<w:QR[{%%Soi%%\"l)ay1V08MS> "
        "$vAfIi(`IBi>ae+X9P=`%%4RLv@Eq'@8Ogr>d^DZR#p?(a4GWTDvb+;Q?l^GS:WK0{"
        "1RQ*Y{ "
        "|?AFZF7R-}$TS~8af:ac)Ju+O/"
        "@K$>MO~;wCc3IB)b)b,suF^3uCCe=OA+bMIV_A}J-'b.K{;VbOr7!ypw16EUrV4(?`"
        "bMVH)j%%6Ts)YJowE`1^%%}Xm*ge~%%Y:*$?MRj:U|V0xZb8/vch+pe!,?t&^/"
        "!d5avmmM<elF8ij1i2@:+w,Q&F^]hU";
  const char delim[]
      = " !\"#$%%&\'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\] "
        "!\"#$%%&\'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]";
  const char *d = delim + 1;
  char *p = f (s, d);
  while (p)
    {
      p = f (0, d);
    }
}

static void __attribute__ ((unused)) a_1021_127 (void)
{
  _1021_127 (&strok_a, TOKEN (strok_a));
}
static void __attribute__ ((unused)) b_1021_127 (void)
{
  _1021_127 (&strok_b, TOKEN (strok_b));
}
static void __attribute__ ((unused)) naive_1021_127 (void)
{
  _1021_127 (&strok_naive, TOKEN (strok_naive));
}

static void __always_inline
all_ascii (char *(*f) (char *restrict s, const char *restrict d),
           const char *fname)
{
  char s0[]
      = "09?i:xg7>MOG\"#w:p^l}^%%,E<xj46H*L\\+L{WXTmTCF2*6{mdL,x+w$]= "
        "7$UMt=xw~C7~zU_h)ej1$8QP$ZXCmfGQM "
        "{eECFgP=iqV\"]51R(SwnnXW{5Yw^&i)@FX\4GU.~N(n8Ds[,BS$3 "
        "8z@uj+ZI?lP.0eU .=F+I//YIGZK49{,;klX.{]x'}k=ucuj_#^\r`?\"`OF*dgB "
        "Xd>m5)UbIm4@\\[hri\\IH:'V-hY_(&hI=#&;nN{SvsG2.<\"rW;l^#da+!Q<u1ufas."
        "\"]emH$~\\WwwF^B9m{ "
        "eXGSG.HvL}p_gT1<j.0?]Xi(5nn/@)vWc5z}+hB^,&B6C3U*V:6+m=inI<`'B "
        "r2Nf~Iscp\"a*gvg~odz!Y%%LK$d-\\M "
        "r&Q:ERDJ<J(k?\"3\"-c*P>X%%</"
        "m`S>iq_e$%%ru%%!u@auqcf]%%);6\"<o-DaS8c&M:g#D$O@.C0q6Rzd.x^9p8h42v1&"
        "P#~9Kl--`0tU^8FGbjM/`OE,L|<<qv./>O6\"(+WG0o\\r/"
        "sIn?ec;J'tU?VDO7n{dvw7P>Nq*HNOPUg)K$%%?3{/"
        "FXkF>Yv99WU]'8&dIX:;~O'1)~;O^83d/R_]Mvi:wjo#rgm^l5t$uat "
        "%%6/I%%6XZLlgWT7SL\\g\"\f|7WF9lw<0f<{[H^f/8-[4/"
        "r`%%gMCEd^gGXba8N\"Iv?e.3:t:\7rz3t.%%M\\j/"
        "Sgh\\Z;hEk~0#HO;1rNC92Y@V<w:QR[{%%Soi%%\"l)ay1V08MS> "
        "$vAfIi(`IBi>ae+X9P=`%%4RLv@Eq'@8Ogr>d^DZR#p?(a4GWTDvb+;Q?l^GS:WK0{"
        "1RQ*Y{ "
        "|?AFZF7R-}$TS~8af:ac)Ju+O/"
        "@K$>MO~;wCc3IB)b)b,suF^3uCCe=OA+bMIV_A}J-'b.K{;VbOr7!ypw16EUrV4(?`"
        "bMVH)j%%6Ts)YJowE`1^%%}Xm*ge~%%Y:*$?MRj:U|V0xZb8/vch+pe!,?t&^/"
        "!d5avmmM<elF8ij1i2@:+w,Q&F^]hU";
  const char d0[]
      = " !\"#$%%&\'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]";
  char s1[]
      = "09?i:xg7>MOG\"#w:p^l}^%%,E<xj46H*L\\+L{WXTmTCF2*6{mdL,x+w$]= "
        "7$UMt=xw~C7~zU_h)ej1$8QP$ZXCmfGQM "
        "{eECFgP=iqV\"]51R(SwnnXW{5Yw^&i)@FX\4GU.~N(n8Ds[,BS$3 "
        "8z@uj+ZI?lP.0eU .=F+I//YIGZK49{,;klX.{]x'}k=ucuj_#^\r`?\"`OF*dgB "
        "Xd>m5)UbIm4@\\[hri\\IH:'V-hY_(&hI=#&;nN{SvsG2.<\"rW;l^#da+!Q<u1ufas."
        "\"]emH$~\\WwwF^B9m{ "
        "eXGSG.HvL}p_gT1<j.0?]Xi(5nn/@)vWc5z}+hB^,&B6C3U*V:6+m=inI<`'B "
        "r2Nf~Iscp\"a*gvg~odz!Y%%LK$d-\\M "
        "r&Q:ERDJ<J(k?\"3\"-c*P>X%%</"
        "m`S>iq_e$%%ru%%!u@auqcf]%%);6\"<o-DaS8c&M:g#D$O@.C0q6Rzd.x^9p8h42v1&"
        "P#~9Kl--`0tU^8FGbjM/`OE,L|<<qv./>O6\"(+WG0o\\r/"
        "sIn?ec;J'tU?VDO7n{dvw7P>Nq*HNOPUg)K$%%?3{/"
        "FXkF>Yv99WU]'8&dIX:;~O'1)~;O^83d/R_]Mvi:wjo#rgm^l5t$uat "
        "%%6/I%%6XZLlgWT7SL\\g\"\f|7WF9lw<0f<{[H^f/8-[4/"
        "r`%%gMCEd^gGXba8N\"Iv?e.3:t:\7rz3t.%%M\\j/"
        "Sgh\\Z;hEk~0#HO;1rNC92Y@V<w:QR[{%%Soi%%\"l)ay1V08MS> "
        "$vAfIi(`IBi>ae+X9P=`%%4RLv@Eq'@8Ogr>d^DZR#p?(a4GWTDvb+;Q?l^GS:WK0{"
        "1RQ*Y{ "
        "|?AFZF7R-}$TS~8af:ac)Ju+O/"
        "@K$>MO~;wCc3IB)b)b,suF^3uCCe=OA+bMIV_A}J-'b.K{;VbOr7!ypw16EUrV4(?`"
        "bMVH)j%%6Ts)YJowE`1^%%}Xm*ge~%%Y:*$?MRj:U|V0xZb8/vch+pe!,?t&^/"
        "!d5avmmM<elF8ij1i2@:+w,Q&F^]hU";
  const char d1[]
      = " !\"#$%%&\'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]";

  char *p0 = strtok (s0, d0), *p1 = f (s1, d1);

  while (p0)
    {
      p0 = strtok (0, d0);
      p1 = f (0, d1);
      assert_equal_str (p0, p1, fname);
    }
  assert_equal_str (p0, p1, fname);
}

static void
test_all_ascii (void)
{
  all_ascii (&strok_a, TOKEN (strok_a));
  all_ascii (&strok_b, TOKEN (strok_b));
  all_ascii (&strok_naive, TOKEN (strok_naive));
}

static void __attribute__ ((unused)) dna_sequencing_test_a (void)
{
  char *delim = " \t\n\v\f\r";
  char sequence[]
      = " ctcaatttga cggcggtaaa gttagatggc tacgcgcgac  aagtctccgt atcgtcatga "
        "aattagcgaa gaggtaatgg   caaagcttgg ctacgaatac aggagcgcgc tgtgattaca  "
        "gtagggttag gatagcgaaa acgttcaacg tggatagact cttatcggca cacgatcata "
        "tgcttccaag gttcccaagg cgaattacta gggtgcacag agctacgagt acgctgtccg "
        "gcttgattcg ctcgtacatc cactgttcaa aaagctccga taccgacgat cactctcgat "
        "ctctgtgtgg gacgcactta ttgtggaatc agtcaaccag tgaagcattc acatgtacgt "
        "ggtacggcac gccgtggtat gttagcgttc cctgcgccgc";

  char *token = strok_a (sequence, delim);
  while (token)
    {
      token = strok_a (0, delim);
    }
}

static void __attribute__ ((unused)) dna_sequencing_test_b (void)
{
  char *delim = " \t\n\v\f\r";
  char sequence[]
      = " ctcaatttga cggcggtaaa gttagatggc tacgcgcgac  aagtctccgt atcgtcatga "
        "aattagcgaa gaggtaatgg   caaagcttgg ctacgaatac aggagcgcgc tgtgattaca  "
        "gtagggttag gatagcgaaa acgttcaacg tggatagact cttatcggca cacgatcata "
        "tgcttccaag gttcccaagg cgaattacta gggtgcacag agctacgagt acgctgtccg "
        "gcttgattcg ctcgtacatc cactgttcaa aaagctccga taccgacgat cactctcgat "
        "ctctgtgtgg gacgcactta ttgtggaatc agtcaaccag tgaagcattc acatgtacgt "
        "ggtacggcac gccgtggtat gttagcgttc cctgcgccgc";

  char *token = strok_b (sequence, delim);
  while (token)
    {
      token = strok_b (0, delim);
    }
}

static void __attribute__ ((unused)) dna_sequencing_test_naive (void)
{
  char *delim = " \t\n\v\f\r";
  char sequence[]
      = " ctcaatttga cggcggtaaa gttagatggc tacgcgcgac  aagtctccgt atcgtcatga "
        "aattagcgaa gaggtaatgg   caaagcttgg ctacgaatac aggagcgcgc tgtgattaca  "
        "gtagggttag gatagcgaaa acgttcaacg tggatagact cttatcggca cacgatcata "
        "tgcttccaag gttcccaagg cgaattacta gggtgcacag agctacgagt acgctgtccg "
        "gcttgattcg ctcgtacatc cactgttcaa aaagctccga taccgacgat cactctcgat "
        "ctctgtgtgg gacgcactta ttgtggaatc agtcaaccag tgaagcattc acatgtacgt "
        "ggtacggcac gccgtggtat gttagcgttc cctgcgccgc";

  char *token = strok_naive (sequence, delim);
  while (token)
    {
      token = strok_naive (0, delim);
    }
}

static void
test_url_b (void)
{
  char url[] = "https://www.website.com/path/to/content/is/"
               "here?q1=soifjas9fsdf&q2=sofisjdofsjdif+3290afn29;`&fj=s";
  char *http_protocol __attribute__ ((unused)) = strok_b (url, "://");
  char *domain __attribute__ ((unused)) = strok_b (0, "/");
  char *path __attribute__ ((unused)) = strok_b (0, "?");
  char *query __attribute__ ((unused));
  while ((query = strok_b (0, "&")))
    ;
}

static void
test_url_naive (void)
{
  char url[] = "https://www.website.com/path/to/content/is/"
               "here?q1=soifjas9fsdf&q2=sofisjdofsjdif+3290afn29;`&fj=s";
  char *http_protocol __attribute__ ((unused)) = strok_naive (url, "://");
  char *domain __attribute__ ((unused)) = strok_naive (0, "/");
  char *path __attribute__ ((unused)) = strok_naive (0, "?");
  char *query __attribute__ ((unused));
  while ((query = strok_naive (0, "&")))
    ;
}

static void
test_url_a (void)
{
  char url[] = "https://www.website.com/path/to/content/is/"
               "here?q1=soifjas9fsdf&q2=sofisjdofsjdif+3290afn29;`&fj=s";
  char *http_protocol __attribute__ ((unused)) = strok_a (url, "://");
  char *domain __attribute__ ((unused)) = strok_a (0, "/");
  char *path __attribute__ ((unused)) = strok_a (0, "?");
  char *query __attribute__ ((unused));
  while ((query = strok_a (0, "&")))
    ;
}

static void
t_strokr_page (char *(*f) (char *restrict s, const char *restrict d,
                           char **pp),
               const char *fname)
{
  unsigned char *buf1, *buf2;
  size_t page_size;

  page_size = 2 * sysconf (_SC_PAGE_SIZE);

  buf1 = mmap (0, (BUF1PAGES + 1) * page_size, PROT_READ | PROT_WRITE,
               MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (buf1 == MAP_FAILED)
    error (EXIT_FAILURE, errno, "mmap failed");
  if (mprotect (buf1 + BUF1PAGES * page_size, page_size, PROT_NONE))
    error (EXIT_FAILURE, errno, "mprotect failed");
  buf2 = mmap (0, 2 * page_size, PROT_READ | PROT_WRITE,
               MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (buf2 == MAP_FAILED)
    error (EXIT_FAILURE, errno, "mmap failed");
  if (mprotect (buf2 + page_size, page_size, PROT_NONE))
    error (EXIT_FAILURE, errno, "mprotect failed");

  memset (buf1, 0xa5, BUF1PAGES * page_size);
  memset (buf2, 0x5a, page_size);

  char line[] = "udf 75868 1 - Live 0xffffffffa0bfb000\n";
  char **saveptrp;
  char *tok;

  saveptrp = (char **)(buf1 + page_size - sizeof (*saveptrp));
  tok = f (line, " \t", saveptrp);
  assert_equal_str (tok, "udf", fname);
}
static void
test_strokr (void)
{
  t_strokr_page (&strokr_a, TOKEN (strokr_a));
  t_strokr_page (&strokr_b, TOKEN (strokr_b));
  t_strokr_page (&strokr_naive, TOKEN (strokr_naive));
}

#define F1 __strtok_r
#define F2 strokr_b
static void
test_align (void)
{
  __attribute__ ((aligned (16))) char s1[1040], s2[1040], d1[1040], d2[1040];

  for (size_t align = 0; align < 16; align++)
    {
      int dec = 1;
      for (size_t size_s = 1024; size_s > 0; size_s >>= 1)
        {
          size_t size_d = size_s >> 1;
          dec = dec ? 0 : 1;

          char *ss1 = memcpy (s1, BUF, 1040), *ss2 = memcpy (s2, BUF, 1040),
               *dd1 = memcpy (d1, BUF + 1040, 1040),
               *dd2 = memcpy (d2, BUF + 1040, 1040);

          ss1 += align;
          ss2 += align;
          dd1 += align;
          dd2 += align;

          ss1[size_s - dec] = 0;
          ss2[size_s - dec] = 0;
          dd1[size_d - dec] = 0;
          dd2[size_d - dec] = 0;

          char *sp1, *sp2;

          char *p1 = F1 (ss1, dd1, &sp1);
          char *p2 = F2 (ss2, dd2, &sp2);

          char msg[32];
          snprintf (msg, 32, "line:%d", __LINE__ + 1);
          assert_equal_str (p1, p2, msg);
          snprintf (msg, 32, "line:%d", __LINE__ + 1);
          assert_equal_str (sp1, sp2, msg);

          do
            {
              p1 = F1 (NULL, dd1, &sp1);
              p2 = F2 (NULL, dd2, &sp2);
              snprintf (msg, 32, "line:%d", __LINE__ + 1);
              assert_equal_str (p1, p2, msg);
              snprintf (msg, 32, "line:%d", __LINE__ + 1);
              assert_equal_str (sp1, sp2, msg);
            }
          while ((p1 != NULL) | (p2 != NULL));

          p1 = F1 (NULL, dd1, &sp1);
          p2 = F2 (NULL, dd2, &sp2);
          snprintf (msg, 32, "line:%d", __LINE__ + 1);
          assert_equal_str (p1, p2, msg);
          snprintf (msg, 32, "line:%d", __LINE__ + 1);
          assert_equal_str (sp1, sp2, msg);
        }
    }
}

int
main (void)
{
  RESUME_PRINTING
  RESUME_COUNTING

  bench_init ();

  test_align ();
  test_basic ();
  test_all_ascii ();
  test_strokr ();
  test_url_a ();
  test_url_b ();
  test_url_naive ();

  if (num_incorrect)
    {
      printf ("\nFailed %d of %d test cases.\n", num_incorrect, total_tests);
    }
  else
    {
      printf ("\nPassed all %d test cases.\n", num_correct);
      assert (num_correct == total_tests);
    }

  // printf ("\nTime Results (n = %d):\n", ITERATIONS);

  // time_test (&test_url_a, 0, TOKEN (test_url_a));
  // time_test (&test_url_b, 0, TOKEN (test_url_b));
  // time_test (&test_url_naive, 0, TOKEN (test_url_naive));

  // time_test (&dna_sequencing_test_a, 0, TOKEN (dna_sequencing_test_a));
  // time_test (&dna_sequencing_test_b, 0, TOKEN (dna_sequencing_test_b));
  // time_test (&dna_sequencing_test_naive, 0, TOKEN
  // (dna_sequencing_test_naive));

  // time_test (&a_1021_127, 0, TOKEN (a_1021_127));
  // time_test (&b_1021_127, 0, TOKEN (b_1021_127));
  // time_test (&naive_1021_127, 0, TOKEN (naive_1021_127));
  return 0;
}
