/* 
**  (C) by Remo Dentato (rdentato@gmail.com)
** 
** This software is distributed under the terms of the MIT license:
**  https://opensource.org/licenses/MIT
**     
**                        ___   __
**                     __/  /_ /  )
**             ___  __(_   ___)  /
**            /  / /  )/  /  /  /
**           /  (_/  //  (__/  / 
**          (____,__/(_____(__/
**    https://github.com/rdentato/clibutl
** 

** This implementation of PEG parser has been inspired by the article:
**    R. R. Redziejowski. Parsing Expression Grammar as a Primitive
**    Recursive-Descent Parser with Backtracking.
**    Fundamenta Informaticae 79, 3-4 (2007), pp. 513 - 524. 
**    http://www.romanredz.se/papers/FI2007.pdf
*/


//<<<//
#ifndef UTL_NOPEG

struct peg_s;
typedef struct peg_s *peg_t;
typedef void (*pegrule_t)(peg_t,const char *);

struct peg_s {
  int32_t      fail;
  int32_t      auxi;
  void        *aux;
  const char  *start;
  const char  *pos;
  vec_t        defer;
  vec_t        memoize; // hastable
  const char  *errpos;
  const char  *errrule;
  int32_t      errln;
  int32_t      errcn;
};

typedef struct  {
  int min;
  int max;
  int rpt;
  const char *pos;
  const char *rpos; 
  int dcnt;
  int rdcnt;
} pegsave_t;
 
typedef int (*pegaction_t)(const char *, const char *, void *);
 
int utl_peg_lower(const char *str);
int utl_peg_oneof(const char *pat, const char *str);
int utl_peg_str(const char *pat, const char *str);

int utl_peg_parse(peg_t p, pegrule_t f, const char *s, const char *r);
#define pegparse(p,f,s) utl_peg_parse(p,PeG_##f,s,#f)

const char *utl_peg_back(peg_t parser,const char *rule_name, const char *pos,int32_t dcnt);
void utl_peg_ref(peg_t parser, const char *rule_name, pegrule_t rule);

peg_t utl_peg_free(peg_t peg);
peg_t utl_peg_new(void);

#define pegnew() utl_peg_new()
#define pegfree(p) utl_peg_free(p)

#define PEG_FAIL     peg_->fail
#define PEG_POS      peg_->pos
#define PEG_AUX      peg_->aux
#define PEG_DCNT     veccount(peg_->defer)

#define PEG_BACK(p_,n_) utl_peg_back(peg_,pegr_,p_,n_)

#define utl_peg_rec(r_) if (!PEG_FAIL) {\
                          PEG_FAIL = r_;\
                          if (PEG_FAIL > 0) { \
                            PEG_POS += PEG_FAIL; \
                            PEG_FAIL = 0;\
                          } \
                        } else (void)0

#define pegis(s_)        utl_peg_reg(s_(PEG_POS,PEG_AUX))
#define pegstr(s_)       utl_peg_rec(utl_peg_str(s_,PEG_POS))
#define pegoneof(s_)     utl_peg_rec(utl_peg_oneof(s_,PEG_POS))
#define peglower         utl_peg_rec(utl_peg_lower(PEG_POS))
#define pegupper         utl_peg_rec((isupper((int)(*PEG_POS))?1:-1))
#define pegdigit         utl_peg_rec((isdigit((int)(*PEG_POS))?1:-1))

// The PEG_BACK() is needed to ensure that errpos is updated (if it needs to be)
#define pegfail        (PEG_FAIL=!PEG_BACK(PEG_POS,PEG_DCNT))
#define pegempty       (PEG_FAIL=!PEG_POS)

#define pegrule(x_)    void PeG_##x_(peg_t peg_, const char *pegr_)
#define pegref(x_)     do { pegrule(x_); utl_peg_ref(peg_,#x_, PeG_##x_); } while (0)

#define peg_do(f_)     int f_(const char *, const char *, void *); \
                       for(pegsave_t peg_save = {.pos=PEG_POS, .dcnt=PEG_DCNT}; \
                            !PEG_FAIL && peg_save.pos; \
                              (PEG_FAIL=(PEG_FAIL?1:f_(peg_save.pos,PEG_POS,PEG_AUX)),\
                              (peg_save.pos=(PEG_FAIL?PEG_BACK(peg_save.pos,peg_save.dcnt):NULL))))
                              
const char *utl_peg_defer(peg_t parser, const char *func,const char *from, const char *to);
#define pegaction(f_)   for(const char *tmp=PEG_POS; \
                            !PEG_FAIL && tmp; \
                              tmp=utl_peg_defer(peg_,#f_,tmp,PEG_POS))

#define pegswitch pegalt
#define pegcase   pegor

#define pegalt    for (pegsave_t peg_save = {.pos=PEG_POS, .dcnt=PEG_DCNT};\
                        !PEG_FAIL && peg_save.pos && (PEG_FAIL=1); \
                          peg_save.pos=PEG_FAIL?PEG_BACK(peg_save.pos,peg_save.dcnt):NULL)
                       
#define pegor      if (PEG_FAIL && !(PEG_FAIL=0) && !PEG_BACK(peg_save.pos,peg_save.dcnt))
#define pegeither  pegor
      
#define pegnot(p_)   for (pegsave_t peg_save = {.pos=PEG_POS, .dcnt=PEG_DCNT}; \
                           !PEG_FAIL && peg_save.pos; \
                             peg_save.pos=(PEG_FAIL=!PEG_FAIL) \
                                           ? PEG_BACK(peg_save.pos,peg_save.dcnt) \
                                           : NULL)
                           
#define pegand       for (pegsave_t peg_save = {.pos=PEG_POS, .dcnt=PEG_DCNT}; \
                           !PEG_FAIL && peg_save.pos;\
                            peg_save.pos=PEG_FAIL?PEG_BACK(peg_save.pos,peg_save.dcnt):NULL)

#define PEG_RPT_SAVE(m_,M_) { .min=m_, .max=M_, .rpt=0, \
                              .pos=PEG_POS, .rpos=PEG_POS, \
                              .dcnt=PEG_DCNT, .rdcnt=PEG_DCNT \
                            }

#define pegrpt(m_,M_) \
   if (PEG_FAIL) (void)0; else \
   for(pegsave_t peg_save = PEG_RPT_SAVE(m_,M_); \
        peg_save.max > 0; \
          peg_save.rpt++,(PEG_FAIL?0:(peg_save.rpos=PEG_POS,peg_save.rdcnt=PEG_DCNT)), \
          ((PEG_FAIL || peg_save.rpt >= peg_save.max) \
            ? (((PEG_FAIL=(peg_save.rpt<=peg_save.min)) \
                 ? PEG_BACK(peg_save.pos,peg_save.dcnt) \
                 : ((PEG_DCNT=peg_save.rdcnt),(PEG_POS=peg_save.rpos))) \
              , peg_save.max=0) \
            : 0)) 
            
#define pegopt   pegrpt(0,1)
#define pegstar  pegrpt(0,INT_MAX)
#define pegmore  pegrpt(1,INT_MAX)


#endif
//>>>//
