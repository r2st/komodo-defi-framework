/******************************************************************************
 * Copyright © 2014-2016 The SuperNET Developers.                             *
 *                                                                            *
 * See the AUTHORS, DEVELOPER-AGREEMENT and LICENSE files at                  *
 * the top-level directory of this distribution for the individual copyright  *
 * holder information and the developer policies on copyright and licensing.  *
 *                                                                            *
 * Unless otherwise agreed in a custom licensing agreement, no part of the    *
 * SuperNET software, including this file may be copied, modified, propagated *
 * or distributed except according to the terms contained in the LICENSE file *
 *                                                                            *
 * Removal or modification of this copyright notice is prohibited.            *
 *                                                                            *
 ******************************************************************************/


//#define uthash_malloc(size) iguana_memalloc(&coin->RThashmem,size,1)
//#define uthash_free(ptr,size)

#include "iguana777.h"
#include "exchanges/bitcoin.h"

struct iguana_hhutxo *iguana_hhutxofind(struct iguana_info *coin,uint64_t uval)
{
    struct iguana_hhutxo *hhutxo;
    HASH_FIND(hh,coin->utxotable,&uval,sizeof(uval),hhutxo);
    return(hhutxo);
}

struct iguana_hhaccount *iguana_hhaccountfind(struct iguana_info *coin,uint64_t pval)
{
    struct iguana_hhaccount *hhacct;
    HASH_FIND(hh,coin->accountstable,&pval,sizeof(pval),hhacct);
    return(hhacct);
}

int32_t iguana_utxoupdate(struct iguana_info *coin,int16_t spent_hdrsi,uint32_t spent_unspentind,uint32_t spent_pkind,uint64_t spent_value,uint32_t spendind,uint32_t fromheight)
{
    //static struct iguana_hhutxo *HHUTXO; static struct iguana_hhaccount *HHACCT; static uint32_t numHHUTXO,maxHHUTXO,numHHACCT,maxHHACCT;
    struct iguana_hhutxo *hhutxo,*tmputxo; struct iguana_hhaccount *hhacct,*tmpacct; uint64_t uval,pval;
    if ( spent_hdrsi < 0 )
    {
        printf(">>>>>>>>>>> RESET UTXO HASH <<<<<<<<<\n");
        if ( coin->utxotable != 0 )
        {
            HASH_ITER(hh,coin->utxotable,hhutxo,tmputxo)
            {
                //HASH_DEL(coin->utxotable,hhutxo);
                hhutxo->u.spentflag = 0;
                hhutxo->u.fromheight = 0;
                hhutxo->u.prevunspentind = 0;
                //free(hhutxo);
            }
            //coin->utxotable = 0;
        }
        if ( coin->accountstable != 0 )
        {
            HASH_ITER(hh,coin->accountstable,hhacct,tmpacct)
            {
                //HASH_DEL(coin->accountstable,hhacct);
                hhacct->a.lastunspentind = 0;
                hhacct->a.total = 0;
                //free(hhacct);
            }
            //coin->accountstable = 0;
        }
        /*if ( HHUTXO != 0 )
        {
            free(HHUTXO);
            maxHHUTXO = numHHUTXO = 0;
            HHUTXO = 0;
        }
        if ( HHACCT != 0 )
        {
            free(HHACCT);
            maxHHACCT = numHHACCT = 0;
            HHACCT = 0;
        }*/
        return(0);
    }
    uval = ((uint64_t)spent_hdrsi << 32) | spent_unspentind;
    pval = ((uint64_t)spent_hdrsi << 32) | spent_pkind;
    if ( (hhutxo= iguana_hhutxofind(coin,uval)) != 0 && hhutxo->u.spentflag != 0 )
    {
        printf("hhutxo.%p spentflag.%d\n",hhutxo,hhutxo->u.spentflag);
        return(-1);
    }
    /*if ( 0 && numHHUTXO+1 >= maxHHUTXO )
    {
        maxHHUTXO += 1;
        HHUTXO = realloc(HHUTXO,sizeof(*HHUTXO) * maxHHUTXO);
    }*/
    hhutxo = calloc(1,sizeof(*hhutxo));//&HHUTXO[numHHUTXO++], memset(hhutxo,0,sizeof(*hhutxo));
    hhutxo->uval = uval;
    HASH_ADD_KEYPTR(hh,coin->utxotable,&hhutxo->uval,sizeof(hhutxo->uval),hhutxo);
    if ( (hhacct= iguana_hhaccountfind(coin,pval)) == 0 )
    {
        /*if ( 0 && numHHACCT+1 >= maxHHACCT )
        {
            maxHHACCT += 1;
            HHACCT = realloc(HHACCT,sizeof(*HHACCT) * maxHHACCT);
        }*/
        hhacct = calloc(1,sizeof(*hhacct)); // &HHACCT[numHHACCT++], memset(hhacct,0,sizeof(*hhacct));
        hhacct->pval = pval;
        HASH_ADD_KEYPTR(hh,coin->accountstable,&hhacct->pval,sizeof(hhacct->pval),hhacct);
    }
    //printf("create hhutxo.%p hhacct.%p from.%d\n",hhutxo,hhacct,fromheight);
    hhutxo->u.spentflag = 1;
    hhutxo->u.fromheight = fromheight;
    hhutxo->u.prevunspentind = hhacct->a.lastunspentind;
    hhacct->a.lastunspentind = spent_unspentind;
    hhacct->a.total += spent_value;
    /*if ( iguana_hhutxofind(coin,uval) == 0 || iguana_hhaccountfind(coin,pval) == 0 )
     {
     printf("null hh find.(%ld %ld) %p %p\n",(long)uval,(long)pval,iguana_hhutxofind(coin,uval),iguana_hhaccountfind(coin,pval));
    }*/
    return(0);
}

int32_t iguana_alloctxbits(struct iguana_info *coin,struct iguana_ramchain *ramchain)
{
    static int64_t total;
    if ( ramchain->txbits == 0 )
    {
        int32_t tlen; uint8_t *TXbits = (uint8_t *)((long)ramchain->H.data + ramchain->H.data->TXoffset);
        tlen = (int32_t)hconv_bitlen(ramchain->H.data->numtxsparse * ramchain->H.data->txsparsebits);
        ramchain->txbits = calloc(1,tlen);
        memcpy(ramchain->txbits,TXbits,tlen);
        total += tlen;
        char str[65]; printf("alloc.[%d] txbits.%p[%d] total %s\n",ramchain->H.data->height/coin->chain->bundlesize,ramchain->txbits,tlen,mbstr(str,total));
        return(tlen);
    }
    return(-1);
}

int32_t iguana_alloccacheT(struct iguana_info *coin,struct iguana_ramchain *ramchain)
{
    static int64_t total;
    if ( ramchain->cacheT == 0 )
    {
        int32_t i,tlen; struct iguana_txid *T = (void *)((long)ramchain->H.data + ramchain->H.data->Toffset);
        tlen = sizeof(*T) * ramchain->H.data->numtxids;
        if ( (ramchain->cacheT= calloc(1,tlen)) != 0 )
        {
            //memcpy(ramchain->cacheT,T,tlen);
            for (i=0; i<ramchain->H.data->numtxids; i++)
                ramchain->cacheT[i] = T[i];
        } else ramchain->cacheT = T;
        total += tlen;
        char str[65]; printf("alloc.[%d] cacheT.%p[%d] total %s\n",ramchain->H.data->height/coin->chain->bundlesize,ramchain->cacheT,tlen,mbstr(str,total));
        return(tlen);
    }
    return(-1);
}

void iguana_volatilesalloc(struct iguana_info *coin,struct iguana_ramchain *ramchain,int32_t copyflag)
{
    int32_t i; struct iguana_utxo *U2; struct iguana_account *A2; struct iguana_ramchaindata *rdata = 0;
    if ( ramchain != 0 && (rdata= ramchain->H.data) != 0 && (coin->current == 0 || coin->current->bundleheight > ramchain->height) )
    {
        //printf("volatilesalloc.[%d] %p %p\n",ramchain->height/coin->chain->bundlesize,ramchain->debitsfileptr,ramchain->lastspendsfileptr);
        if ( ramchain->allocatedA2 == 0 )
        {
            ramchain->A2 = calloc(sizeof(*ramchain->A2),rdata->numpkinds + 16);
            ramchain->allocatedA2 = sizeof(*ramchain->A2) * rdata->numpkinds;
        }
        if ( ramchain->allocatedU2 == 0 )
        {
            ramchain->Uextras = calloc(sizeof(*ramchain->Uextras),rdata->numunspents + 16);
            ramchain->allocatedU2 = sizeof(*ramchain->Uextras) * rdata->numunspents;
        }
        if ( ramchain->debitsfileptr != 0 )
        {
            if ( copyflag != 0 )
            {
                A2 = (void *)((long)ramchain->debitsfileptr + sizeof(int32_t) + 2*sizeof(bits256));
                if ( ramchain->debitsfilesize != sizeof(int32_t) + 2*sizeof(bits256) + sizeof(*A2)*rdata->numpkinds )
                    printf("A2 size mismatch %ld != %ld\n",ramchain->debitsfilesize,sizeof(int32_t) + 2*sizeof(bits256) + sizeof(*A2)*rdata->numpkinds);
                for (i=0; i<rdata->numpkinds; i++)
                    ramchain->A2[i] = A2[i];
            }
            munmap(ramchain->debitsfileptr,ramchain->debitsfilesize);
            ramchain->debitsfileptr = 0;
            ramchain->debitsfilesize = 0;
        }
        if ( ramchain->lastspendsfileptr != 0 )
        {
            if ( copyflag != 0 )
            {
                U2 = (void *)((long)ramchain->lastspendsfileptr + sizeof(int32_t) + 2*sizeof(bits256));
                if ( ramchain->lastspendsfilesize != sizeof(int32_t) + 2*sizeof(bits256) + sizeof(*U2)*rdata->numunspents )
                    printf("U2 size mismatch %ld != %ld\n",ramchain->lastspendsfilesize,sizeof(int32_t) + 2*sizeof(bits256) + sizeof(*U2)*rdata->numunspents);
                for (i=0; i<rdata->numunspents; i++)
                    ramchain->Uextras[i] = U2[i];
            }
            munmap(ramchain->lastspendsfileptr,ramchain->lastspendsfilesize);
            ramchain->lastspendsfileptr = 0;
            ramchain->lastspendsfilesize = 0;
        }
    } else printf("illegal ramchain.%p rdata.%p\n",ramchain,rdata);
}

void iguana_volatilespurge(struct iguana_info *coin,struct iguana_ramchain *ramchain)
{
    if ( ramchain != 0 )
    {
        //printf("volatilespurge.[%d] (%p %p) %p %p\n",ramchain->height/coin->chain->bundlesize,ramchain->A2,ramchain->Uextras,ramchain->debitsfileptr,ramchain->lastspendsfileptr);
        if ( ramchain->allocatedA2 != 0 && ramchain->A2 != 0 && ramchain->A2 != ramchain->debitsfileptr+sizeof(bits256)*2+sizeof(int32_t) )
            free(ramchain->A2);
        if ( ramchain->allocatedU2 != 0 && ramchain->Uextras != 0 && ramchain->Uextras != ramchain->lastspendsfileptr+sizeof(bits256)*2+sizeof(int32_t) )
            free(ramchain->Uextras);
        ramchain->A2 = 0;
        ramchain->Uextras = 0;
        ramchain->allocatedA2 = ramchain->allocatedU2 = 0;
        if ( ramchain->debitsfileptr != 0 )
        {
            munmap(ramchain->debitsfileptr,ramchain->debitsfilesize);
            ramchain->debitsfileptr = 0;
            ramchain->debitsfilesize = 0;
        }
        if ( ramchain->lastspendsfileptr != 0 )
        {
            munmap(ramchain->lastspendsfileptr,ramchain->lastspendsfilesize);
            ramchain->lastspendsfileptr = 0;
            ramchain->lastspendsfilesize = 0;
        }
    }
}

int32_t iguana_volatilesmap(struct iguana_info *coin,struct iguana_ramchain *ramchain)
{
    int32_t iter,numhdrsi,err = -1; char fname[1024]; bits256 balancehash,allbundles; struct iguana_ramchaindata *rdata;
    if ( (rdata= ramchain->H.data) == 0 )
    {
        printf("volatilesmap.[%d] no rdata\n",ramchain->height/coin->chain->bundlesize);
        return(-1);
    }
    for (iter=0; iter<2; iter++)
    {
        sprintf(fname,"%s/%s%s/accounts/debits.%d",GLOBAL_DBDIR,iter==0?"ro/":"",coin->symbol,ramchain->height);
        if ( (ramchain->debitsfileptr= OS_mapfile(fname,&ramchain->debitsfilesize,0)) != 0 && ramchain->debitsfilesize == sizeof(int32_t) + 2*sizeof(bits256) + sizeof(*ramchain->A2) * ramchain->H.data->numpkinds )
        {
            ramchain->from_roA = (iter == 0);
            numhdrsi = *(int32_t *)ramchain->debitsfileptr;
            memcpy(balancehash.bytes,(void *)((long)ramchain->debitsfileptr + sizeof(numhdrsi)),sizeof(balancehash));
            memcpy(allbundles.bytes,(void *)((long)ramchain->debitsfileptr + sizeof(numhdrsi) + sizeof(balancehash)),sizeof(allbundles));
            if ( coin->balanceswritten == 0 )
            {
                coin->balanceswritten = numhdrsi;
                coin->balancehash = balancehash;
                coin->allbundles = allbundles;
            }
            if ( numhdrsi == coin->balanceswritten && memcmp(balancehash.bytes,coin->balancehash.bytes,sizeof(balancehash)) == 0 && memcmp(allbundles.bytes,coin->allbundles.bytes,sizeof(allbundles)) == 0 )
            {
                ramchain->A2 = (void *)((long)ramchain->debitsfileptr + sizeof(numhdrsi) + 2*sizeof(bits256));
                sprintf(fname,"%s/%s%s/accounts/lastspends.%d",GLOBAL_DBDIR,iter==0?"ro/":"",coin->symbol,ramchain->height);
                if ( (ramchain->lastspendsfileptr= OS_mapfile(fname,&ramchain->lastspendsfilesize,0)) != 0 && ramchain->lastspendsfilesize == sizeof(int32_t) + 2*sizeof(bits256) + sizeof(*ramchain->Uextras) * ramchain->H.data->numunspents )
                {
                    numhdrsi = *(int32_t *)ramchain->lastspendsfileptr;
                    memcpy(balancehash.bytes,(void *)((long)ramchain->lastspendsfileptr + sizeof(numhdrsi)),sizeof(balancehash));
                    memcpy(allbundles.bytes,(void *)((long)ramchain->lastspendsfileptr + sizeof(numhdrsi) + sizeof(balancehash)),sizeof(allbundles));
                    if ( numhdrsi == coin->balanceswritten && memcmp(balancehash.bytes,coin->balancehash.bytes,sizeof(balancehash)) == 0 && memcmp(allbundles.bytes,coin->allbundles.bytes,sizeof(allbundles)) == 0 )
                    {
                        ramchain->Uextras = (void *)((long)ramchain->lastspendsfileptr + sizeof(numhdrsi) + 2*sizeof(bits256));
                        ramchain->from_roU = (iter == 0);
                        //printf("volatilesmap.[%d] %p %p\n",ramchain->height/coin->chain->bundlesize,ramchain->debitsfileptr,ramchain->lastspendsfileptr);
                        err = 0;
                    } else printf("ramchain map error2 balanceswritten %d vs %d hashes %x %x\n",coin->balanceswritten,numhdrsi,coin->balancehash.uints[0],balancehash.uints[0]);
                } else printf("ramchain map error3 %s\n",fname);
            }
            else
            {
                printf("ramchain.[%d] map error balanceswritten %d vs %d hashes %x %x\n",ramchain->H.data->height,coin->balanceswritten,numhdrsi,coin->balancehash.uints[0],balancehash.uints[0]);
                err++;
                OS_removefile(fname,0);
            }
        }
        if ( err == 0 )
            return(0);
    }
    //printf("couldnt map [%d]\n",ramchain->height/coin->chain->bundlesize);
    iguana_volatilespurge(coin,ramchain);
    return(err);
}

int32_t iguana_spentflag(struct iguana_info *coin,int64_t *RTspendp,int32_t *spentheightp,struct iguana_ramchain *ramchain,int16_t spent_hdrsi,uint32_t spent_unspentind,int32_t height,uint64_t amount)
{
    uint32_t numunspents; struct iguana_hhutxo *hhutxo; struct iguana_utxo utxo; uint64_t val;
    *spentheightp = 0;
    numunspents = ramchain->H.data->numunspents;
    memset(&utxo,0,sizeof(utxo));
    val = ((uint64_t)spent_hdrsi << 32) | spent_unspentind;
    if ( spent_unspentind != 0 && spent_unspentind < numunspents )
    {
        if ( ramchain->Uextras != 0 )
            utxo = ramchain->Uextras[spent_unspentind];
        if ( ramchain->Uextras == 0 || utxo.spentflag == 0 )
        {
            //printf("check hhutxo [%d] u%u %p\n",spent_hdrsi,spent_unspentind,iguana_hhutxofind(coin,((uint64_t)202<<32)|3909240));
            if ( (hhutxo= iguana_hhutxofind(coin,val)) != 0 )
            {
                utxo = hhutxo->u;
                if ( utxo.spentflag != 0 )
                    (*RTspendp) += amount;
            }
        }
    }
    else
    {
        printf("illegal unspentind.%u vs %u hdrs.%d\n",spent_unspentind,numunspents,spent_hdrsi);
        return(-1);
    }
    if ( utxo.spentflag != 0 && utxo.fromheight == 0 )
    {
        printf("illegal unspentind.%u vs %u hdrs.%d zero fromheight?\n",spent_unspentind,numunspents,spent_hdrsi);
        return(-1);
    }
    //printf("[%d] u%u %.8f, spentheight.%d vs height.%d spentflag.%d\n",spent_hdrsi,spent_unspentind,dstr(amount),utxo.fromheight,height,utxo.spentflag);
    *spentheightp = utxo.fromheight;
    if ( height == 0 || utxo.fromheight < height )
        return(utxo.spentflag);
    else return(0);
}

int32_t iguana_volatileupdate(struct iguana_info *coin,int32_t incremental,struct iguana_ramchain *spentchain,int16_t spent_hdrsi,uint32_t spent_unspentind,uint32_t spent_pkind,uint64_t spent_value,uint32_t spendind,uint32_t fromheight)
{
    struct iguana_account *A2; struct iguana_ramchaindata *rdata; struct iguana_utxo *utxo;
    if ( (rdata= spentchain->H.data) != 0 )
    {
        if ( incremental == 0 )
        {
            if ( spentchain->Uextras != 0 && (A2= spentchain->A2) != 0 )
            {
                utxo = &spentchain->Uextras[spent_unspentind];
                if ( utxo->spentflag == 0 )
                {
                    if ( 0 && fromheight/coin->chain->bundlesize >= coin->current->hdrsi )
                        printf("iguana_volatileupdate.%d: [%d] spent.(u%u %.8f pkind.%d) fromht.%d [%d] spendind.%d\n",incremental,spent_hdrsi,spent_unspentind,dstr(spent_value),spent_pkind,fromheight,fromheight/coin->chain->bundlesize,spendind);
                    utxo->prevunspentind = A2[spent_pkind].lastunspentind;
                    utxo->spentflag = 1;
                    utxo->fromheight = fromheight;
                    A2[spent_pkind].total += spent_value;
                    A2[spent_pkind].lastunspentind = spent_unspentind;
                    return(0);
                }
                else
                {
                    printf("from.%d spent_unspentind[%d] in hdrs.[%d] is spent fromht.%d %.8f\n",fromheight,spent_unspentind,spent_hdrsi,utxo->fromheight,dstr(spent_value));
                }
            } else printf("null ptrs.[%d] u.%u p.%u %.8f from ht.%d s.%u\n",spent_hdrsi,spent_unspentind,spent_pkind,dstr(spent_value),fromheight,spendind);
        }
        else // do the equivalent of historical, ie mark as spent, linked list, balance
        {
            //double startmillis = OS_milliseconds(); static double totalmillis; static int32_t utxon;
            if ( iguana_utxoupdate(coin,spent_hdrsi,spent_unspentind,spent_pkind,spent_value,spendind,fromheight) == 0 )
            {
                /*totalmillis += (OS_milliseconds() - startmillis);
                if ( (++utxon % 100000) == 0 )
                    printf("ave utxo[%d] %.2f micros total %.2f seconds\n",utxon,(1000. * totalmillis)/utxon,totalmillis/1000.);*/
                return(0);
            }
        }
        printf("iguana_volatileupdate.%d: [%d] spent.(u%u %.8f pkind.%d) double spend? at ht.%d [%d] spendind.%d (%p %p)\n",incremental,spent_hdrsi,spent_unspentind,dstr(spent_value),spent_pkind,fromheight,fromheight/coin->chain->bundlesize,spendind,spentchain->Uextras,spentchain->A2);
        if ( coin->current != 0 && fromheight >= coin->current->bundleheight )
            coin->RTdatabad = 1;
        else
        {
            printf("from.%d vs current.%d\n",fromheight,coin->current->bundleheight);
            iguana_bundleremove(coin,spent_hdrsi,0);
            iguana_bundleremove(coin,fromheight/coin->chain->bundlesize,0);
        }
        exit(-1);
    } else printf("volatileupdate error null rdata [%d]\n",spentchain->height/coin->current->bundleheight);
    return(-1);
}

uint32_t iguana_sparseadd(uint8_t *bits,uint32_t ind,int32_t width,uint32_t tablesize,uint8_t *key,int32_t keylen,uint32_t setind,void *refdata,int32_t refsize,struct iguana_ramchain *ramchain,uint32_t maxitems)
{
    static uint8_t masks[8] = { 1, 2, 4, 8, 16, 32, 64, 128 };
    int32_t i,j,x,n,modval; int64_t bitoffset; uint8_t *ptr; uint32_t *table,retval = 0;
    if ( tablesize == 0 )
    {
        printf("iguana_sparseadd tablesize zero illegal\n");
        return(0);
    }
    if ( 0 && setind == 0 )
    {
        char str[65];
        for (i=n=0; i<tablesize; i++)
        {
            bitoffset = (i * width);
            ptr = &bits[bitoffset >> 3];
            modval = (bitoffset & 7);
            for (x=j=0; j<width; j++,modval++)
            {
                if ( modval >= 8 )
                    ptr++, modval = 0;
                x <<= 1;
                x |= (*ptr & masks[modval]) >> modval;
            }
            if ( x != 0 )
                printf("%s ",bits256_str(str,*(bits256 *)(refdata + x*refsize))), n++;
        }
        printf("tableentries.%d\n",n);
    }
    //if ( setind == 0 )
    //    ramchain->sparsesearches++;
    //else ramchain->sparseadds++;
    if ( 0 && (ramchain->sparsesearches % 1000000) == 0 )
        printf("[%3d] %7d.[%-2d %8d] %5.3f adds.(%-10ld %10ld) search.(hits.%-10ld %10ld) %5.2f%% max.%ld\n",ramchain->height/ramchain->H.data->numblocks,ramchain->height,width,tablesize,(double)(ramchain->sparseadditers + ramchain->sparsesearchiters)/(1+ramchain->sparsesearches+ramchain->sparseadds),ramchain->sparseadds,ramchain->sparseadditers,ramchain->sparsehits,ramchain->sparsesearches,100.*(double)ramchain->sparsehits/(1+ramchain->sparsesearches),ramchain->sparsemax+1);
    if ( width == 32 )
    {
        table = (uint32_t *)bits;
        for (i=0; i<tablesize; i++,ind++)
        {
            if ( ind >= tablesize )
                ind = 0;
            if ( (x= table[ind]) == 0 )
            {
                //if ( ++i > ramchain->sparsemax )
                //    ramchain->sparsemax = i;
                if ( (retval= setind) != 0 )
                {
                    //ramchain->sparseadditers += i;
                    table[ind] = setind;
                } //else ramchain->sparsesearchiters += i;
                return(setind);
            }
            else if ( x < maxitems && memcmp((void *)(long)((long)refdata + x*refsize),key,keylen) == 0 )
            {
                if ( setind != 0 && setind != x )
                    printf("sparseadd index collision setind.%d != x.%d refsize.%d keylen.%d\n",setind,x,refsize,keylen);
                //ramchain->sparsehits++;
                //if ( ++i > ramchain->sparsemax )
                //    ramchain->sparsemax = i;
                //ramchain->sparseadditers += i;
                return(x);
            }
        }
    }
    else
    {
        bitoffset = (ind * width);
        if ( 0 && setind == 0 )
            printf("tablesize.%d width.%d bitoffset.%d\n",tablesize,width,(int32_t)bitoffset);
        for (i=0; i<tablesize; i++,ind++,bitoffset+=width)
        {
            if ( ind >= tablesize )
            {
                ind = 0;
                bitoffset = 0;
            }
            x = 0;
            if ( width == 32 )
                memcpy(&x,&bits[bitoffset >> 3],4);
            else if ( width == 16 )
                memcpy(&x,&bits[bitoffset >> 3],2);
            else if ( width != 8 )
            {
                ptr = &bits[bitoffset >> 3];
                modval = (bitoffset & 7);
                if ( 0 && setind == 0 )
                    printf("tablesize.%d width.%d bitoffset.%d modval.%d i.%d\n",tablesize,width,(int32_t)bitoffset,modval,i);
                for (x=j=0; j<width; j++,modval++)
                {
                    if ( modval >= 8 )
                        ptr++, modval = 0;
                    x <<= 1;
                    x |= (*ptr & masks[modval]) >> modval;
                }
            }
            else x = bits[bitoffset >> 3];
            if ( 0 && setind == 0 )
                printf("x.%d\n",x);
            if ( x == 0 )
            {
                if ( (x= setind) == 0 )
                {
                    //ramchain->sparsesearchiters += (i+1);
                    return(0);
                }
                //else ramchain->sparseadditers += (i+1);
                if ( width == 32 )
                    memcpy(&bits[bitoffset >> 3],&setind,4);
                else if ( width == 16 )
                    memcpy(&bits[bitoffset >> 3],&setind,2);
                else if ( width != 8 )
                {
                    ptr = &bits[(bitoffset+width-1) >> 3];
                    modval = ((bitoffset+width-1) & 7);
                    for (j=0; j<width; j++,x>>=1,modval--)
                    {
                        if ( modval < 0 )
                            ptr--, modval = 7;
                        if ( (x & 1) != 0 )
                            *ptr |= masks[modval];
                    }
                }
                else bits[bitoffset >> 3] = setind;
                if ( 0 )
                {
                    for (x=j=0; j<width; j++)
                    {
                        x <<= 1;
                        x |= GETBIT(bits,bitoffset+width-1-j) != 0;
                    }
                    //if ( x != setind )
                    printf("x.%u vs setind.%d ind.%d bitoffset.%d, width.%d\n",x,setind,ind,(int32_t)bitoffset,width);
                }
                //if ( i > ramchain->sparsemax )
                //    ramchain->sparsemax = i;
                return(setind);
            }
            else if ( x < maxitems && memcmp((void *)(long)((long)refdata + x*refsize),key,keylen) == 0 )
            {
                if ( setind == 0 )
                    ramchain->sparsehits++;
                else if ( setind != x )
                    printf("sparseadd index collision setind.%d != x.%d refsize.%d keylen.%d\n",setind,x,refsize,keylen);
                if ( i > ramchain->sparsemax )
                    ramchain->sparsemax = i;
                return(x);
            }
        }
    }
    return(0);
}

uint32_t iguana_sparseaddtx(uint8_t *bits,int32_t width,uint32_t tablesize,bits256 txid,struct iguana_txid *T,uint32_t txidind,struct iguana_ramchain *ramchain)
{
    uint32_t ind,retval;
    //char str[65]; printf("sparseaddtx %s txidind.%d bits.%p\n",bits256_str(str,txid),txidind,bits);
    ind = (txid.ulongs[0] ^ txid.ulongs[1] ^ txid.ulongs[2] ^ txid.ulongs[3]) % tablesize;
    if ( (retval= iguana_sparseadd(bits,ind,width,tablesize,txid.bytes,sizeof(txid),txidind,T,sizeof(*T),ramchain,ramchain->H.data->numtxids)) != 0 )
    {
        char str[65];
        if ( txidind != 0 && retval != txidind )
            printf("sparse tx collision %s %u vs %u\n",bits256_str(str,txid),retval,txidind);
        return(retval);
    }
    return(retval);
}

uint32_t iguana_sparseaddpk(uint8_t *bits,int32_t width,uint32_t tablesize,uint8_t rmd160[20],struct iguana_pkhash *P,uint32_t pkind,struct iguana_ramchain *ramchain)
{
    uint32_t ind,key2; uint64_t key0,key1;
    //int32_t i; for (i=0; i<20; i++)
    //    printf("%02x",rmd160[i]);
    //char str[65]; printf(" sparseaddpk pkind.%d bits.%p\n",pkind,bits);
    memcpy(&key0,rmd160,sizeof(key0));
    memcpy(&key1,&rmd160[sizeof(key0)],sizeof(key1));
    memcpy(&key2,&rmd160[sizeof(key0) + sizeof(key1)],sizeof(key2));
    ind = (key0 ^ key1 ^ key2) % tablesize;
    return(iguana_sparseadd(bits,ind,width,tablesize,rmd160,20,pkind,P,sizeof(*P),ramchain,ramchain->H.data->numpkinds));
}

int32_t iguana_ramchain_spendtxid(struct iguana_info *coin,uint32_t *unspentindp,bits256 *txidp,struct iguana_txid *T,int32_t numtxids,bits256 *X,int32_t numexternaltxids,struct iguana_spend *s)
{
    uint32_t ind,external;
    *unspentindp = 0;
    memset(txidp,0,sizeof(*txidp));
    ind = s->spendtxidind;
    external = (ind >> 31) & 1;
    ind &= ~(1 << 31);
    //printf("s.%p ramchaintxid vout.%x spendtxidind.%d isext.%d ext.%d ind.%d\n",s,s->prevout,ind,s->external,external,ind);
    if ( s->prevout < 0 )
        return(-1);
    if ( s->external != 0 && s->external == external && ind < numexternaltxids )
    {
        //printf("ind.%d X.%p[%d]\n",ind,X,numexternaltxids);
        *txidp = X[ind];
        return(s->prevout);
    }
    else if ( s->external == 0 && s->external == external && ind < numtxids )
    {
        *txidp = T[ind].txid;
        *unspentindp = T[ind].firstvout + s->prevout;
        return(s->prevout);
    }
    return(-2);
}

struct iguana_txid *iguana_txidfind(struct iguana_info *coin,int32_t *heightp,struct iguana_txid *tx,bits256 txid,int32_t lasthdrsi)
{
    uint8_t *TXbits; struct iguana_txid *T; uint32_t txidind; int32_t i;
    struct iguana_bundle *bp; struct iguana_ramchain *ramchain; //struct iguana_block *block;
    *heightp = -1;
    if ( lasthdrsi < 0 )
        return(0);
    for (i=lasthdrsi; i>=0; i--)
    {
        if ( (bp= coin->bundles[i]) != 0 && bp->emitfinish > 1 )
        {
            ramchain = &bp->ramchain;//(bp->isRT != 0) ? &coin->RTramchain : &bp->ramchain;
            if ( ramchain->H.data != 0 )
            {
                if ( (TXbits= ramchain->txbits) == 0 )
                {
                    if ( coin->PREFETCHLAG >= 0 && coin->fastfind == 0 )
                        iguana_alloctxbits(coin,ramchain);
                    if ( (TXbits= ramchain->txbits) == 0 )
                    {
                        //printf("use memory mapped.[%d]\n",ramchain->H.data->height/coin->chain->bundlesize);
                        TXbits = (void *)(long)((long)ramchain->H.data + ramchain->H.data->TXoffset);
                    }
                }
                if ( (T= ramchain->cacheT) == 0 )
                {
                    if ( coin->PREFETCHLAG >= 0 && coin->fastfind == 0 )
                        iguana_alloccacheT(coin,ramchain);
                    if ( (T= ramchain->cacheT) == 0 )
                        T = (void *)(long)((long)ramchain->H.data + ramchain->H.data->Toffset);
                }
                if ( (txidind= iguana_sparseaddtx(TXbits,ramchain->H.data->txsparsebits,ramchain->H.data->numtxsparse,txid,T,0,ramchain)) > 0 )
                {
                    //printf("found txidind.%d\n",txidind);
                    if ( bits256_cmp(txid,T[txidind].txid) == 0 )
                    {
                        if ( 0 )
                        {
                            int32_t j; struct iguana_block *block;
                            for (j=0; j<bp->n; j++)
                                if ( (block= bp->blocks[j]) != 0 && txidind >= block->RO.firsttxidind && txidind < block->RO.firsttxidind+block->RO.txn_count )
                                    break;
                            if ( j < bp->n )
                            {
                                if ( j != T[txidind].bundlei )
                                    printf("bundlei mismatch j.%d != %d\n",j,T[txidind].bundlei);
                                else
                                {
                                    *heightp = bp->bundleheight + T[txidind].bundlei;
                                    //printf("found height.%d\n",*heightp);
                                    *tx = T[txidind];
                                    return(tx);
                                }
                            }
                        }
                        else
                        {
                            *heightp = bp->bundleheight + T[txidind].bundlei;
                            //printf("found height.%d\n",*heightp);
                            *tx = T[txidind];
                            return(tx);
                        }
                    }
                    char str[65],str2[65]; printf("iguana_txidfind mismatch.[%d:%d] %d %s vs %s\n",bp->hdrsi,T[txidind].extraoffset,txidind,bits256_str(str,txid),bits256_str(str2,T[txidind].txid));
                    return(0);
                }
            }
        }
    }
    return(0);
}

struct iguana_bundle *iguana_externalspent(struct iguana_info *coin,bits256 *prevhashp,uint32_t *unspentindp,struct iguana_ramchain *ramchain,int32_t spent_hdrsi,struct iguana_spend *s,int32_t prefetchflag)
{
    int32_t prev_vout,height,hdrsi; uint32_t sequenceid,unspentind; char str[65];
    struct iguana_bundle *spentbp=0; struct iguana_txid *T,TX,*tp; bits256 *X; bits256 prev_hash;
    X = (void *)(long)((long)ramchain->H.data + ramchain->H.data->Xoffset);
    T = (void *)(long)((long)ramchain->H.data + ramchain->H.data->Toffset);
    //printf("external X.%p %ld num.%d\n",X,(long)ramchain->H.data->Xoffset,(int32_t)ramchain->H.data->numexternaltxids);
    sequenceid = s->sequenceid;
    hdrsi = spent_hdrsi;
    *unspentindp = 0;
    memset(prevhashp,0,sizeof(*prevhashp));
    if ( s->prevout < 0 )
    {
        //printf("n.%d coinbase at spendind.%d firstvin.%d -> firstvout.%d -> unspentind\n",m,spendind,nextT->firstvin,nextT->firstvout);
        //nextT++;
        //m++;
        return(0);
    }
    else
    {
        prev_vout = s->prevout;
        iguana_ramchain_spendtxid(coin,&unspentind,&prev_hash,T,ramchain->H.data->numtxids,X,ramchain->H.data->numexternaltxids,s);
        *prevhashp = prev_hash;
        *unspentindp = unspentind;
        if ( unspentind == 0 )
        {
            //double duration,startmillis = OS_milliseconds();
            if ( (tp= iguana_txidfind(coin,&height,&TX,prev_hash,spent_hdrsi-1)) != 0 )
            {
                *unspentindp = unspentind = TX.firstvout + ((prev_vout > 0) ? prev_vout : 0);
                hdrsi = height / coin->chain->bundlesize;
                if ( hdrsi >= 0 && hdrsi < coin->bundlescount && (spentbp= coin->bundles[hdrsi]) != 0 )
                {
                    //printf("%s height.%d firstvout.%d prev.%d ->U%d\n",bits256_str(str,prev_hash),height,TX.firstvout,prev_vout,unspentind);
                    /*now = (uint32_t)time(NULL);
                    duration = (OS_milliseconds() - startmillis);
                    if ( 0 && ((uint64_t)coin->txidfind_num % 1000000) == 1 )
                        printf("%p iguana_txidfind.[%.0f] ave %.2f micros, total %.2f seconds | duration %.3f millis\n",spentbp->ramchain.txbits,coin->txidfind_num,(coin->txidfind_totalmillis*1000.)/coin->txidfind_num,coin->txidfind_totalmillis/1000.,duration);
                    coin->txidfind_totalmillis += duration;
                    coin->txidfind_num += 1.;*/
                    if ( 1 && coin->PREFETCHLAG > 0 )
                    {
                        if ( spentbp->lastprefetch == 0 )
                        {
                            iguana_ramchain_prefetch(coin,&spentbp->ramchain,prefetchflag);
                            spentbp->lastprefetch = (uint32_t)time(NULL);
                        }
                        /*else if ( 0 && (rand() % IGUANA_NUMHELPERS) == 0 && (duration > 10 || duration > (10 * coin->txidfind_totalmillis)/coin->txidfind_num) )
                        {
                            printf("slow txidfind %.2f vs %.2f prefetch[%d] from.[%d] lag.%ld last.%u\n",duration,coin->txidfind_totalmillis/coin->txidfind_num,spentbp->hdrsi,ramchain->height/coin->chain->bundlesize,time(NULL) - spentbp->lastprefetch,spentbp->lastprefetch);
                            iguana_ramchain_prefetch(coin,ramchain,1);
                            //spentbp->lastprefetch = now;
                        }*/
                    }
                }
                else
                {
                    printf("illegal hdrsi.%d prev_hash.(%s) for bp.[%d]\n",hdrsi,bits256_str(str,prev_hash),spent_hdrsi);
                    exit(-1);
                }
            }
            else
            {
                printf("cant find prev_hash.(%s) for bp.[%d]\n",bits256_str(str,prev_hash),spent_hdrsi);
                if ( spent_hdrsi < coin->current->hdrsi )
                {
                    iguana_bundleremove(coin,spent_hdrsi,1);
                    exit(-1);
                }
                coin->RTdatabad = 1;
                return(0);
            }
        } else printf("external spent unexpected nonz unspentind [%d]\n",spent_hdrsi);
    }
    if ( (spentbp= coin->bundles[hdrsi]) == 0 || hdrsi > spent_hdrsi )
        printf("illegal hdrsi.%d when [%d] spentbp.%p\n",hdrsi,spent_hdrsi,spentbp);
    else if ( unspentind == 0 || unspentind >= spentbp->ramchain.H.data->numunspents )
        printf("illegal unspentind.%d vs max.%d spentbp.%p[%d]\n",unspentind,spentbp->ramchain.H.data->numunspents,spentbp,hdrsi);
    else return(spentbp);
    iguana_bundleremove(coin,spent_hdrsi,1);
    exit(-1);
    return(0);
}

int32_t iguana_txidfastfind(struct iguana_info *coin,int32_t *heightp,bits256 txid,int32_t lasthdrsi)
{
    uint8_t *sorted,*item; int32_t i,j,val,num,tablesize,*hashtable; uint32_t firstvout;
    if ( (sorted= coin->fast[txid.bytes[31]]) != 0 )
    {
        memcpy(&num,sorted,sizeof(num));
        memcpy(&tablesize,&sorted[sizeof(num)],sizeof(tablesize));
        if ( (hashtable= coin->fasttables[txid.bytes[31]]) == 0 )
        {
            hashtable = (int32_t *)((long)sorted + (1 + num)*16);
            //printf("backup hashtable\n");
        }
        val = (txid.uints[4] % tablesize);
        for (j=0; j<tablesize; j++,val++)
        {
            if ( val >= tablesize )
                val = 0;
            if ( (i= hashtable[val]) == 0 )
                return(-1);
            else
            {
                if ( i > num )
                {
                    printf("illegal val.%d vs num.%d tablesize.%d fastfind.%02x\n",i,num,tablesize,txid.bytes[31]);
                    return(-1);
                }
                else
                {
                    item = (void *)((long)sorted + i*16);
                    if ( memcmp(&txid.txid,item,sizeof(uint64_t)) == 0 )
                    {
                        memcpy(&firstvout,&item[sizeof(uint64_t)],sizeof(firstvout));
                        memcpy(heightp,&item[sizeof(uint64_t) + sizeof(firstvout)],sizeof(*heightp));
                        //printf("i.%d val.%d height.%d firstvout.%d j.%d\n",i,val,*heightp,firstvout,j);
                        if ( *heightp >= (lasthdrsi+1)*coin->chain->bundlesize )
                        {
                            printf("txidfastfind: unexpected height.%d with lasthdrsi.%d\n",*heightp,lasthdrsi);
                            return(-1);
                        }
                        return(firstvout);
                    }
                    else if ( 0 )
                    {
                        int32_t k;
                        for (k=-16; k<0; k++)
                            printf("%02x ",item[k]);
                        printf("<");
                        for (k=0; k<16; k++)
                            printf("%02x ",item[k]);
                        printf(">");
                        for (k=16; k<32; k++)
                            printf("%02x ",item[k]);
                        printf("\n");
                        printf("txid.%llx vs item.%llx ht.%d 1st.%d\n",(long long)txid.txid,*(long long *)item,*(int32_t *)&item[sizeof(uint64_t)],*(int32_t *)&item[sizeof(uint64_t)+sizeof(uint32_t)]);
                    }
                }
            }
        }
    }
    return(-1);
}

int32_t iguana_unspentindfind(struct iguana_info *coin,int32_t *heightp,bits256 txid,int32_t vout,int32_t lasthdrsi)
{
    struct iguana_txid *tp,TX; int32_t firstvout;
    if ( coin->fastfind != 0 && (firstvout= iguana_txidfastfind(coin,heightp,txid,lasthdrsi)) >= 0 )
    {
        return(firstvout + vout);
    }
    else
    {
        if ( (tp= iguana_txidfind(coin,heightp,&TX,txid,lasthdrsi)) != 0 )
            return(tp->firstvout + vout);
    }
    return(-1);
}

int32_t iguana_fastfindadd(struct iguana_info *coin,bits256 txid,int32_t height,uint32_t firstvout)
{
    FILE *fp;
    if ( bits256_nonz(txid) != 0 && (fp= coin->fastfps[txid.bytes[31]]) != 0 )
    {
        txid.uints[6] = firstvout;
        txid.uints[7] = height;
        if ( fwrite(&txid,1,sizeof(txid),fp) == sizeof(txid) )
            return(1);
    }
    return(0);
}

int64_t iguana_fastfindinitbundle(struct iguana_info *coin,struct iguana_bundle *bp,int32_t iter)
{
    int32_t i; struct iguana_txid *T; struct iguana_ramchaindata *rdata; int64_t n = 0;
    if ( (rdata= bp->ramchain.H.data) != 0 )
    {
        T = (void *)(long)((long)rdata + rdata->Toffset);
        n = rdata->numtxids;
        if ( iter == 1 )
        {
            for (i=0; i<n; i++)
                iguana_fastfindadd(coin,T[i].txid,bp->bundleheight + T[i].bundlei,T[i].firstvout);
            fprintf(stderr,"[%d:%u] ",bp->hdrsi,(int32_t)n);
        }
    }
    return(n);
}

static int _bignum_cmp(const void *a,const void *b)
{
    uint8_t *biga,*bigb; int32_t i,diff;
    biga = (uint8_t *)a;
    bigb = (uint8_t *)b;
    for (i=0; i<32; i++)
    {
        if ( (diff= (biga[i] - bigb[i])) > 0 )
            return(1);
        else if ( diff < 0 )
            return(-1);
    }
    return(0);
}

uint32_t iguana_fastfindinit(struct iguana_info *coin)
{
    int32_t i,j,iter,num,tablesize,*hashtable; uint8_t *sorted; char fname[1024];
    for (iter=0; iter<2; iter++)
    {
        for (i=0; i<0x100; i++)
        {
            sprintf(fname,"DB/%s%s/fastfind/%02x.all",iter==0?"ro/":"",coin->symbol,i), OS_compatible_path(fname);
            if ( (coin->fast[i]= OS_mapfile(fname,&coin->fastsizes[i],0)) == 0 )
                break;
            else
            {
                fprintf(stderr,".");
                sorted = coin->fast[i];
                if ( 0 )
                {
                    coin->fast[i] = calloc(1,coin->fastsizes[i]);
                    memcpy(coin->fast[i],sorted,coin->fastsizes[i]);
                    munmap(sorted,coin->fastsizes[i]);
                }
                sorted = coin->fast[i];
                memcpy(&num,sorted,sizeof(num));
                memcpy(&tablesize,&sorted[sizeof(num)],sizeof(tablesize));
                if ( (num+1)*16 + tablesize*sizeof(*hashtable) == coin->fastsizes[i] )
                {
                    hashtable = (int32_t *)((long)sorted + (1 + num)*16);
                    if ( 0 )
                    {
                        coin->fasttables[i] = calloc(tablesize,sizeof(*hashtable));
                        memcpy(coin->fasttables[i],hashtable,tablesize * sizeof(*hashtable));
                    }
                }
                else
                {
                    printf("size error num.%d tablesize.%d -> %lu vs %ld\n",num,tablesize,(num+1)*16 + tablesize*sizeof(*hashtable),coin->fastsizes[i]);
                    break;
                }
            }
        }
        if ( i == 0x100 )
        {
            coin->fastfind = (uint32_t)time(NULL);
            printf("initialized fastfind.%s iter.%d\n",coin->symbol,iter);
            return(coin->fastfind);
        }
        else
        {
            for (j=0; j<i; j++)
            {
                munmap(coin->fast[i],coin->fastsizes[i]);
                free(coin->fasttables[i]);
                coin->fast[i] = 0;
                coin->fastsizes[i] = 0;
            }
        }
    }
    return(0);
}

int64_t iguana_fastfindcreate(struct iguana_info *coin)
{
    int32_t i,j,val,iter,errs,num,ind,tablesize,*hashtable; bits256 *sortbuf,hash2; long allocsize; struct iguana_bundle *bp; char fname[512]; uint8_t buf[16]; int64_t total = 0;
    if ( coin->current != 0 && coin->bundlescount == coin->current->hdrsi+1 )
    {
        sprintf(fname,"DB/%s/fastfind",coin->symbol), OS_ensure_directory(fname);
        for (i=0; i<0x100; i++)
        {
            sprintf(fname,"DB/%s/fastfind/%02x",coin->symbol,i), OS_compatible_path(fname);
            if ( (coin->fastfps[i]= fopen(fname,"wb")) == 0 )
                break;
        }
        if ( i == 0x100 )
        {
            for (iter=0; iter<2; iter++)
            {
                total = 0;
                for (i=0; i<coin->bundlescount-1; i++)
                    if ( (bp= coin->bundles[i]) != 0 )
                        total += iguana_fastfindinitbundle(coin,bp,iter);
                printf("iguana_fastfindinit iter.%d total.%lld\n",iter,(long long)total);
            }
            for (i=errs=0; i<0x100; i++)
            {
                fclose(coin->fastfps[i]);
                sprintf(fname,"DB/%s/fastfind/%02x",coin->symbol,i), OS_compatible_path(fname);
                //printf("%s\n",fname);
                if ( (sortbuf= OS_filestr(&allocsize,fname)) != 0 )
                {
                    num = (int32_t)allocsize/sizeof(bits256);
                    qsort(sortbuf,num,sizeof(bits256),_bignum_cmp);
                    strcat(fname,".all");
                    if ( (coin->fastfps[i]= fopen(fname,"wb")) != 0 )
                    {
                        tablesize = (num << 1);
                        hashtable = calloc(sizeof(*hashtable),tablesize);
                        for (ind=1; ind<=num; ind++)
                        {
                            hash2 = sortbuf[ind-1];
                            val = (hash2.uints[4] % tablesize);
                            for (j=0; j<tablesize; j++,val++)
                            {
                                if ( val >= tablesize )
                                    val = 0;
                                if ( hashtable[val] == 0 )
                                {
                                    hashtable[val] = ind;
                                    break;
                                }
                            }
                        }
                        memset(&hash2,0,sizeof(hash2));
                        hash2.uints[0] = num;
                        hash2.uints[1] = tablesize;
                        for (j=0; j<=num; j++)
                        {
                            memcpy(buf,&hash2.txid,sizeof(hash2.txid));
                            memcpy(&buf[sizeof(hash2.txid)],&hash2.uints[6],sizeof(hash2.uints[6]));
                            memcpy(&buf[sizeof(hash2.txid) + sizeof(hash2.uints[6])],&hash2.uints[7],sizeof(hash2.uints[7]));
                            fwrite(buf,1,sizeof(buf),coin->fastfps[i]);
                            //fwrite(hash2,1,sizeof(hash2),coin->fastfps[i]);
                            if ( j < num )
                            {
                                hash2 = sortbuf[j];
                                //char str[65]; printf("%d %s\n",j,bits256_str(str,hash2));
                            }
                        }
                        if ( fwrite(hashtable,sizeof(*hashtable),tablesize,coin->fastfps[i]) == tablesize )
                        {
                            fclose(coin->fastfps[i]);
                            coin->fastfps[i] = 0;
                            if ( (coin->fast[i]= OS_mapfile(fname,&coin->fastsizes[i],0)) != 0 )
                            {
                            } else errs++;
                            printf("%s fastfind.[%02x] num.%d tablesize.%d errs.%d %p[%ld]\n",fname,i,num,tablesize,errs,coin->fast[i],coin->fastsizes[i]);
                        }
                        else
                        {
                            printf("error saving (%s)\n",fname);
                            OS_removefile(fname,0);
                            fclose(coin->fastfps[i]);
                            coin->fastfps[i] = 0;
                        }
                        free(hashtable);
                    } else printf("couldnt overwrite (%s)\n",fname);
                    free(sortbuf);
                } else printf("couldnt load sortbuf (%s)\n",fname);
            }
            printf("initialized with errs.%d\n",errs);
            if ( errs == 0 )
                coin->fastfind = (uint32_t)time(NULL);
        }
    }
    return(total);
}

struct iguana_bundle *iguana_fastexternalspent(struct iguana_info *coin,bits256 *prevhashp,uint32_t *unspentindp,struct iguana_ramchain *ramchain,int32_t spent_hdrsi,struct iguana_spend *s)
{
    int32_t prev_vout,height,hdrsi,firstvout; uint32_t ind;
    struct iguana_txid *T; bits256 *X; bits256 prev_hash; struct iguana_ramchaindata *rdata;
    if ( (rdata= ramchain->H.data) == 0 )
        return(0);
    hdrsi = spent_hdrsi;
    *unspentindp = 0;
    memset(prevhashp,0,sizeof(*prevhashp));
    if ( (prev_vout= s->prevout) >= 0 )
    {
        ind = s->spendtxidind & ~(1 << 31);
        if ( s->external != 0 )
        {
            if ( ind < rdata->numexternaltxids )
            {
                char str[65]; //double duration,startmillis = OS_milliseconds();
                X = (void *)(long)((long)rdata + rdata->Xoffset);
                *prevhashp = prev_hash = X[ind];
                if ( (firstvout= iguana_txidfastfind(coin,&height,prev_hash,spent_hdrsi-1)) >= 0 )
                {
                    /*duration = (OS_milliseconds() - startmillis);
                    if ( ((uint64_t)coin->txidfind_num % 100) == 1 )
                        printf("[%d] iguana_fasttxidfind.[%.0f] ave %.2f micros, total %.2f seconds | duration %.3f millis\n",spent_hdrsi,coin->txidfind_num,(coin->txidfind_totalmillis*1000.)/coin->txidfind_num,coin->txidfind_totalmillis/1000.,duration);
                    coin->txidfind_totalmillis += duration;
                    coin->txidfind_num += 1.;*/

                    *unspentindp = firstvout + prev_vout;
                    hdrsi = height / coin->chain->bundlesize;
                    if ( hdrsi >= 0 && hdrsi < coin->bundlescount )
                        return(coin->bundles[hdrsi]);
                } else printf("couldnt find (%s)\n",bits256_str(str,prev_hash));
            } else return(0);
        }
        else if ( ind < rdata->numtxids )
        {
            T = (void *)(long)((long)rdata + rdata->Toffset);
            *prevhashp = T[ind].txid;
            *unspentindp = T[ind].firstvout + s->prevout;
            return(coin->bundles[hdrsi]);
        }
    }
    return(0);
}

cJSON *ramchain_unspentjson(struct iguana_unspent *up,uint32_t unspentind)
{
    cJSON *item = cJSON_CreateObject();
    jaddnum(item,"hdrsi",up->hdrsi);
    jaddnum(item,"pkind",up->pkind);
    jaddnum(item,"unspentind",unspentind);
    jaddnum(item,"prevunspentind",up->prevunspentind);
    jadd64bits(item,"satoshis",up->value);
    jaddnum(item,"txidind",up->txidind);
    jaddnum(item,"vout",up->vout);
    jaddnum(item,"type",up->type);
    jaddnum(item,"fileid",up->fileid);
    jaddnum(item,"scriptpos",up->scriptpos);
    jaddnum(item,"scriptlen",up->scriptlen);
    return(item);
}

cJSON *iguana_unspentjson(struct iguana_info *coin,int32_t hdrsi,uint32_t unspentind,struct iguana_txid *T,struct iguana_unspent *up,uint8_t rmd160[20],char *coinaddr,uint8_t *pubkey33)
{
    /*{
     "txid" : "d54994ece1d11b19785c7248868696250ab195605b469632b7bd68130e880c9a",
     "vout" : 1,
     "address" : "mgnucj8nYqdrPFh2JfZSB1NmUThUGnmsqe",
     "account" : "test label",
     "scriptPubKey" : "76a9140dfc8bafc8419853b34d5e072ad37d1a5159f58488ac",
     "amount" : 0.00010000,
     "confirmations" : 6210,
     "spendable" : true
     },*/
    //struct iguana_unspent { uint64_t value; uint32_t txidind,pkind,prevunspentind; uint16_t hdrsi:12,type:4,vout; } __attribute__((packed));
    struct iguana_waccount *wacct; int32_t height,ind; char scriptstr[8192],asmstr[sizeof(scriptstr)+1024]; cJSON *item; uint32_t checkind;
    item = cJSON_CreateObject();
    jaddbits256(item,"txid",T[up->txidind].txid);
    jaddnum(item,"vout",up->vout);
    jaddstr(item,"address",coinaddr);
    if ( iguana_scriptget(coin,scriptstr,asmstr,sizeof(scriptstr),hdrsi,unspentind,T[up->txidind].txid,up->vout,rmd160,up->type,pubkey33) != 0 )
        jaddstr(item,"scriptPubKey",scriptstr);
    jaddnum(item,"amount",dstr(up->value));
    if ( (checkind= iguana_unspentindfind(coin,&height,T[up->txidind].txid,up->vout,coin->bundlescount-1)) != 0 )
    {
        jaddnum(item,"confirmations",coin->longestchain - height);
        jaddnum(item,"checkind",checkind);
    }
    if ( (wacct= iguana_waddressfind(coin,&ind,coinaddr)) != 0 )
    {
        jaddstr(item,"account",wacct->account);
        jadd(item,"spendable",jtrue());
    } else jadd(item,"spendable",jfalse());
    jadd(item,"unspent",ramchain_unspentjson(up,unspentind));
    return(item);
}

struct iguana_pkhash *iguana_pkhashfind(struct iguana_info *coin,struct iguana_ramchain **ramchainp,int64_t *depositsp,uint32_t *lastunspentindp,struct iguana_pkhash *p,uint8_t rmd160[20],int32_t firsti,int32_t endi)
{
    uint8_t *PKbits; struct iguana_pkhash *P; uint32_t pkind,numpkinds,i; struct iguana_bundle *bp; struct iguana_ramchain *ramchain; struct iguana_ramchaindata *rdata; struct iguana_account *ACCTS;
    *depositsp = 0;
    *ramchainp = 0;
    *lastunspentindp = 0;
    for (i=firsti; i<coin->bundlescount&&i<=endi; i++)
    {
        if ( (bp= coin->bundles[i]) != 0 )
        {
            if ( coin->RTramchain_busy != 0 )
            {
                printf("iguana_pkhashfind: unexpected access when RTramchain_busy\n");
                return(0);
            }
            ramchain = (bp != coin->current) ? &bp->ramchain : &coin->RTramchain;
            if ( (rdata= ramchain->H.data) != 0 )
            {
                numpkinds = rdata->numpkinds;
                PKbits = (void *)(long)((long)rdata + rdata->PKoffset);
                P = (void *)(long)((long)rdata + rdata->Poffset);
                if ( bp == coin->current )
                    ACCTS = ramchain->A;
                else ACCTS = (void *)(long)((long)rdata + rdata->Aoffset);
                if ( (pkind= iguana_sparseaddpk(PKbits,rdata->pksparsebits,rdata->numpksparse,rmd160,P,0,ramchain)) > 0 && pkind < numpkinds )
                {
                    *ramchainp = ramchain;
                    *depositsp = ACCTS[pkind].total;
                    *lastunspentindp = ACCTS[pkind].lastunspentind;
                    *p = P[pkind];
printf("[%d] return pkind.%u %.8f last.%u ACCTS.%p %p\n",i,pkind,dstr(*depositsp),*lastunspentindp,ACCTS,ramchain->A);
                    return(p);
                }
                else if ( pkind != 0 )
                    printf("[%d] not found pkind.%d vs num.%d RT.%d rdata.%p\n",i,pkind,rdata->numpkinds,bp->isRT,rdata);
            } else printf("%s.[%d] error null rdata isRT.%d\n",coin->symbol,i,bp->isRT);
        }
    }
    return(0);
}

char *iguana_bundleaddrs(struct iguana_info *coin,int32_t hdrsi)
{
    uint8_t *PKbits; struct iguana_pkhash *P; uint32_t pkind,numpkinds; struct iguana_bundle *bp; struct iguana_ramchain *ramchain; cJSON *retjson; char rmdstr[41];
    if ( (bp= coin->bundles[hdrsi]) != 0 )
    {
        if ( coin->RTramchain_busy != 0 )
        {
            printf("iguana_bundleaddrs: unexpected access when RTramchain_busy\n");
            return(0);
        }
        ramchain = &bp->ramchain;//(bp->isRT != 0) ? &bp->ramchain : &coin->RTramchain;
        if ( ramchain->H.data != 0 )
        {
            numpkinds = ramchain->H.data->numpkinds;//(bp->isRT != 0) ? ramchain->H.data->numpkinds : ramchain->pkind;
            retjson = cJSON_CreateArray();
            PKbits = (void *)(long)((long)ramchain->H.data + ramchain->H.data->PKoffset);
            P = (void *)(long)((long)ramchain->H.data + ramchain->H.data->Poffset);
            for (pkind=0; pkind<numpkinds; pkind++,P++)
            {
                init_hexbytes_noT(rmdstr,P->rmd160,20);
                jaddistr(retjson,rmdstr);
            }
            return(jprint(retjson,1));
        }
        //iguana_bundleQ(coin,bp,bp->n);
        return(clonestr("{\"error\":\"no bundle data\"}"));
    } return(clonestr("{\"error\":\"no bundle\"}"));
}

int32_t iguana_uheight(struct iguana_info *coin,int32_t bundleheight,struct iguana_txid *T,int32_t numtxids,struct iguana_unspent *up)
{
    if ( up->txidind > 0 && up->txidind < numtxids )
        return(bundleheight + T[up->txidind].bundlei);
    else return(bundleheight);
}

int64_t iguana_pkhashbalance(struct iguana_info *coin,cJSON *array,int64_t *spentp,int32_t *nump,struct iguana_ramchain *ramchain,struct iguana_pkhash *p,uint32_t lastunspentind,uint8_t rmd160[20],char *coinaddr,uint8_t *pubkey33,int32_t hdrsi,int32_t height)
{
    struct iguana_unspent *U; struct iguana_utxo *U2; struct iguana_spend *S; int32_t uheight,spentheight; uint32_t pkind=0,unspentind; int64_t spent = 0,checkval,deposits = 0; struct iguana_txid *T; struct iguana_account *A2; struct iguana_ramchaindata *rdata = 0; int64_t RTspend = 0;
    *spentp = *nump = 0;
    if ( coin->RTramchain_busy != 0 )
    {
        printf("iguana_pkhashbalance: unexpected access when RTramchain_busy\n");
        return(0);
    }
    if ( ramchain->Uextras == 0 || (rdata= ramchain->H.data) == 0 )
    {
        printf("iguana_pkhashbalance: unexpected null spents.%p or rdata.%p\n",ramchain->Uextras,rdata);
        return(0);
    }
    unspentind = lastunspentind;
    U = (void *)(long)((long)rdata + rdata->Uoffset);
    T = (void *)(long)((long)rdata + rdata->Toffset);
    RTspend = 0;
    while ( unspentind > 0 )
    {
        (*nump)++;
        uheight = iguana_uheight(coin,ramchain->height,T,rdata->numtxids,&U[unspentind]);
        if ( uheight < height )
        {
            deposits += U[unspentind].value;
            if ( iguana_spentflag(coin,&RTspend,&spentheight,ramchain,hdrsi,unspentind,height,U[unspentind].value) == 0 )
            {
                if ( array != 0 )
                    jaddi(array,iguana_unspentjson(coin,hdrsi,unspentind,T,&U[unspentind],rmd160,coinaddr,pubkey33));
            } else spent += U[unspentind].value;
            if ( p->pkind != U[unspentind].pkind )
                printf("warning: [%d] p->pkind.%u vs U->pkind.%u for u%d\n",hdrsi,p->pkind,U[unspentind].pkind,unspentind);
        }
        pkind = p->pkind;
        unspentind = U[unspentind].prevunspentind;
    }
    if ( (A2= ramchain->A2) != 0 && (U2= ramchain->Uextras) != 0 )
    {
        S = (void *)(long)((long)rdata + rdata->Soffset);
        unspentind = A2[pkind].lastunspentind;
        checkval = 0;
        while ( unspentind > 0 )
        {
            uheight = iguana_uheight(coin,ramchain->height,T,rdata->numtxids,&U[unspentind]);
            if ( uheight < height )
            {
                checkval += U[unspentind].value;
                //printf("u%u %.8f spentflag.%d prev.%u fromheight.%d\n",unspentind,dstr(U[unspentind].value),U2[unspentind].spentflag,U2[unspentind].prevunspentind,U2[unspentind].fromheight);
            }
            unspentind = U2[unspentind].prevunspentind;
        }
        if ( fabs(spent - checkval - RTspend) > SMALLVAL )
            printf("spend %s: [%d] deposits %.8f spent %.8f check %.8f (%.8f) vs A2[%u] %.8f\n",height==0?"checkerr":"",hdrsi,dstr(deposits),dstr(spent),dstr(checkval)+dstr(RTspend),dstr(*spentp),pkind,dstr(A2[pkind].total));
    }
    (*spentp) = spent;
    //printf("spent %.8f, RTspent %.8f deposits %.8f\n",dstr(spent),dstr(RTspend),dstr(deposits));
    return(deposits - spent);
}

int32_t iguana_pkhasharray(struct iguana_info *coin,cJSON *array,int32_t minconf,int32_t maxconf,int64_t *totalp,struct iguana_pkhash *P,int32_t max,uint8_t rmd160[20],char *coinaddr,uint8_t *pubkey33,int32_t lastheight)
{
    int32_t i,n,m; int64_t spent,deposits,netbalance,total; uint32_t lastunspentind; struct iguana_ramchain *ramchain;
    if ( coin->RTramchain_busy != 0 )
    {
        printf("iguana_pkhasharray: unexpected access when RTramchain_busy\n");
        return(-1);
    }
    for (total=i=n=0; i<max && i<coin->bundlescount; i++)
    {
        if ( i > lastheight/coin->chain->bundlesize )
            break;
        if ( iguana_pkhashfind(coin,&ramchain,&deposits,&lastunspentind,&P[n],rmd160,i,i) != 0 )
        {
            if ( (netbalance= iguana_pkhashbalance(coin,array,&spent,&m,ramchain,&P[n],lastunspentind,rmd160,coinaddr,pubkey33,i,lastheight)) != deposits-spent )
            {
                printf("pkhash balance mismatch from m.%d check %.8f vs %.8f spent %.8f [%.8f]\n",m,dstr(netbalance),dstr(deposits),dstr(spent),dstr(deposits)-dstr(spent));
            }
            else
            {
                //printf("pkhash balance.[%d] from m.%d check %.8f vs %.8f spent %.8f [%.8f]\n",i,m,dstr(netbalance),dstr(deposits),dstr(spent),dstr(deposits)-dstr(spent));
                total += netbalance;
                n++;
            }
        }
        //printf("%d: balance %.8f, lastunspent.%u\n",i,dstr(balance),lastunspentind);
    }
    //printf("n.%d max.%d\n",n,max);
    *totalp = total;
    return(n);
}

void iguana_unspents(struct supernet_info *myinfo,struct iguana_info *coin,cJSON *array,int32_t minconf,int32_t maxconf,uint8_t *rmdarray,int32_t numrmds)
{
    int64_t total,sum=0; struct iguana_pkhash *P; uint8_t *addrtypes,*pubkeys; int32_t i,flag = 0; char coinaddr[64];
    if ( coin->RTramchain_busy != 0 )
    {
        printf("iguana_pkhasharray: unexpected access when RTramchain_busy\n");
        return;
    }
    if ( rmdarray == 0 )
        rmdarray = iguana_walletrmds(myinfo,coin,&numrmds), flag++;
    addrtypes = &rmdarray[numrmds * 20], pubkeys = &rmdarray[numrmds * 21];
    P = calloc(coin->bundlescount,sizeof(*P));
    for (i=0; i<numrmds; i++)
    {
        bitcoin_address(coinaddr,addrtypes[i],&rmdarray[i * 20],20);
        iguana_pkhasharray(coin,array,minconf,maxconf,&total,P,coin->bundlescount,&rmdarray[i * 20],coinaddr,&pubkeys[33*i],0);
        printf("i.%d of %d: %s %.8f\n",i,numrmds,coinaddr,dstr(total));
        sum += total;
    }
    printf("sum %.8f\n",dstr(sum));
    free(P);
    if ( flag != 0 )
        free(rmdarray);
}

static inline int32_t _iguana_spendvectorconv(struct iguana_spendvector *ptr,struct iguana_unspent *u,int32_t numpkinds,int32_t hdrsi,uint32_t unspentind)
{
    uint32_t spent_pkind = 0;
    if ( (spent_pkind= u->pkind) != 0 && spent_pkind < numpkinds )
    {
        ptr->pkind = spent_pkind;
        ptr->value = u->value;
        ptr->tmpflag = 0;
        return(spent_pkind);
    } else printf("spendvectorconv [%d] u%d pkind.%u/num %u\n",hdrsi,unspentind,u->pkind,numpkinds);
    return(0);
}

uint32_t iguana_spendvectorconv(struct iguana_info *coin,struct iguana_spendvector *ptr,struct iguana_bundle *bp)
{
    static uint64_t count,converted,errs;
    struct iguana_bundle *spentbp; struct iguana_unspent *spentU; uint32_t spent_pkind;
    count++;
    if ( 0 && (count % 1000000) == 0 )
        printf("iguana_spendvectorconv.[%llu] errs.%llu converted.%llu %.2f%%\n",(long long)count,(long long)errs,(long long)converted,100. * (long long)converted/count);
    if ( ptr->tmpflag != 0 )
    {
        if ( ptr->hdrsi >= 0 && ptr->hdrsi < coin->bundlescount && (spentbp= coin->bundles[ptr->hdrsi]) != 0 )
        {
            spentU = (void *)(long)((long)spentbp->ramchain.H.data + spentbp->ramchain.H.data->Uoffset);
            if ( (spent_pkind= _iguana_spendvectorconv(ptr,&spentU[ptr->unspentind],spentbp->ramchain.H.data->numpkinds,ptr->hdrsi,ptr->unspentind)) != 0 )
                converted++;
            else printf("illegal [%d].u%u pkind.%u vs %u\n",ptr->hdrsi,ptr->unspentind,spent_pkind,spentbp->ramchain.H.data->numpkinds);
        } else printf("illegal [%d].u%u\n",ptr->hdrsi,ptr->unspentind);
        errs++;
        return(0);
    } //else printf("[%d] tmpflag.%d u%d %.8f p%u\n",ptr->hdrsi,ptr->tmpflag,ptr->unspentind,dstr(ptr->value),ptr->pkind);
    return(ptr->pkind);
}

int32_t iguana_spendvectorsave(struct iguana_info *coin,struct iguana_bundle *bp,struct iguana_ramchain *ramchain,struct iguana_spendvector *ptr,int32_t emit,int32_t n)
{
    int32_t i,retval = -1; FILE *fp; char fname[1024],str[65]; long fsize; bits256 zero,sha256;
    if ( ptr == 0 || (bp->hdrsi != 0 && ptr == bp->ramchain.Xspendinds) )
    {
        //printf("iguana_spendvectorsave.[%d] ptr.%p Xspendinds\n",bp->hdrsi,ptr);
        return(0);
    }
    memset(zero.bytes,0,sizeof(zero));
    for (i=0; i<emit; i++)
        if ( iguana_spendvectorconv(coin,&ptr[i],bp) == 0 )
        {
            printf("iguana_spendvectorconv error [%d] at %d of %d/%d\n",bp->hdrsi,i,emit,n);
            return(-1);
        }
    sprintf(fname,"%s/%s/spends/%s.%d",GLOBAL_DBDIR,coin->symbol,bits256_str(str,bp->hashes[0]),bp->bundleheight);
    vcalc_sha256(0,sha256.bytes,(void *)ptr,(int32_t)(sizeof(*ptr) * emit));
    if ( (fp= fopen(fname,"wb")) != 0 )
    {
        if ( fwrite(sha256.bytes,1,sizeof(sha256),fp) != sizeof(sha256) )
            printf("error writing hash for %ld -> (%s)\n",sizeof(*ptr) * emit,fname);
        else if ( fwrite(ptr,sizeof(*ptr),emit,fp) != emit )
            printf("error writing %d of %d -> (%s)\n",emit,n,fname);
        else
        {
            retval = 0;
            fsize = ftell(fp);
            fclose(fp), fp = 0;
            bp->Xvalid = 0;
            if ( iguana_Xspendmap(coin,ramchain,bp) < 0 )
                printf("error mapping Xspendmap.(%s)\n",fname);
            else
            {
                printf("created.(%s) %p[%d]\n",fname,bp->ramchain.Xspendinds,bp->ramchain.numXspends);
                retval = 0;
            }
        }
        if ( fp != 0 )
            fclose(fp);
        //int32_t i; for (i=0; i<ramchain->numXspends; i++)
        //    printf("(%d u%d) ",ramchain->Xspendinds[i].hdrsi,ramchain->Xspendinds[i].ind);
        //printf("filesize %ld Xspendptr.%p %p num.%d\n",fsize,ramchain->Xspendptr,ramchain->Xspendinds,ramchain->numXspends);
    }
    else printf("iguana_spendvectors: Error creating.(%s)\n",fname);
    return(retval);
}

int32_t iguana_spendvectors(struct iguana_info *coin,struct iguana_bundle *bp,struct iguana_ramchain *ramchain,int32_t starti,int32_t numblocks,int32_t convertflag,int32_t iterate)
{
    static uint64_t total,emitted;
    int32_t iter,spendind,n=0,txidind,errs=0,emit=0,i,j,k; double startmillis; bits256 prevhash;
    uint32_t spent_unspentind,spent_pkind,now,starttime; struct iguana_ramchaindata *rdata;
    struct iguana_bundle *spentbp; struct iguana_blockRO *B; struct iguana_spendvector *ptr;
    struct iguana_unspent *u,*spentU;  struct iguana_txid *T; char str[65];
    struct iguana_spend *S,*s; //void *fastfind = 0;
    //printf("iguana_spendvectors.[%d] gen.%d ramchain data.%p txbits.%p\n",bp->hdrsi,bp->bundleheight,ramchain->H.data,ramchain->txbits);
    if ( (rdata= ramchain->H.data) == 0 || (n= rdata->numspends) < 1 )
    {
        printf("iguana_spendvectors: no rdata.%p %d\n",rdata,n);
        return(0);
    }
    B = (void *)(long)((long)rdata + rdata->Boffset);
    S = (void *)(long)((long)rdata + rdata->Soffset);
    T = (void *)(long)((long)rdata + rdata->Toffset);
    if ( ramchain->Xspendinds != 0 )
    {
        bp->tmpspends = ramchain->Xspendinds;
        bp->numtmpspends = ramchain->numXspends;
        bp->utxofinish = (uint32_t)time(NULL);
        bp->balancefinish = 0;
        //printf("iguana_spendvectors.[%d]: already have Xspendinds[%d]\n",bp->hdrsi,ramchain->numXspends);
        return(0);
    }
    ptr = mycalloc('x',sizeof(*ptr),n);
    total += n;
    startmillis = OS_milliseconds();
    if ( 0 && strcmp(coin->symbol,"BTC") == 0 )
        printf("start UTXOGEN.%d max.%d ptr.%p millis.%.3f\n",bp->bundleheight,n,ptr,startmillis);
    starttime = (uint32_t)time(NULL);
    iguana_ramchain_prefetch(coin,&bp->ramchain,3);
    for (iter=0; iter<=iterate; iter++)
    {
        if ( iterate != 0 )
        {
            //fastfind = coin->fast[iter];
            //coin->fast[iter] = calloc(1,coin->fastsizes[iter]);
            //memcpy(coin->fast[iter],fastfind,coin->fastsizes[iter]);
        }
        txidind = B[starti].firsttxidind;
        spendind = B[starti].firstvin;
        for (i=starti; i<numblocks; i++)
        {
            if ( txidind != B[i].firsttxidind || spendind != B[i].firstvin )
            {
                printf("spendvectors: txidind %u != %u B[%d].firsttxidind || spendind %u != %u B[%d].firstvin\n",txidind,B[i].firsttxidind,i,spendind,B[i].firstvin,i);
                myfree(ptr,sizeof(*ptr) * n);
                return(-1);
            }
            for (j=0; j<B[i].txn_count && errs==0; j++,txidind++)
            {
                now = (uint32_t)time(NULL);
                if ( txidind != T[txidind].txidind || spendind != T[txidind].firstvin )
                {
                    printf("spendvectors: txidind %u != %u nextT[txidind].firsttxidind || spendind %u != %u nextT[txidind].firstvin\n",txidind,T[txidind].txidind,spendind,T[txidind].firstvin);
                    myfree(ptr,sizeof(*ptr) * n);
                    return(-1);
                }
                for (k=0; k<T[txidind].numvins && errs==0; k++,spendind++)
                {
#ifdef __APPLE__
                    if ( bp == coin->current && (spendind % 10000) == 0 )
                        printf("iter.%02x [%-3d:%4d] spendvectors elapsed t.%-3d spendind.%d\n",iter,bp->hdrsi,i,(uint32_t)time(NULL)-starttime,spendind);
#endif
                    u = 0;
                    spentbp = 0;
                    s = &S[spendind];
                    if ( s->external != 0 && s->prevout >= 0 )
                    {
                        if ( coin->fastfind != 0 )
                        {
                            spentbp = iguana_fastexternalspent(coin,&prevhash,&spent_unspentind,ramchain,bp->hdrsi,s);
                        }
                        else if ( spentbp == 0 )
                        {
                            if ( (spentbp= iguana_externalspent(coin,&prevhash,&spent_unspentind,ramchain,bp->hdrsi,s,2)) != 0 )
                            {
                                if ( coin->fastfind != 0 )
                                    printf("found prevhash using slow, not fast\n");
                            }
                        }
                        if ( iterate != 0 && (spentbp == 0 || spentbp->hdrsi != iter) )
                            continue;
                        if ( bits256_nonz(prevhash) == 0 )
                            continue;
                        if ( spentbp != 0 && spentbp->ramchain.H.data != 0 )
                        {
                            if ( spentbp == bp )
                            {
                                char str[65];
                                printf("unexpected spendbp: height.%d bp.[%d] U%d <- S%d.[%d] [ext.%d %s prev.%d]\n",bp->bundleheight+i,spentbp->hdrsi,spent_unspentind,spendind,bp->hdrsi,s->external,bits256_str(str,prevhash),s->prevout);
                                errs++;
                                break;
                            }
                            if ( convertflag != 0 )
                            {
                                if ( coin->PREFETCHLAG > 0 && now >= spentbp->lastprefetch+coin->PREFETCHLAG )
                                {
                                    printf("prefetch[%d] from.[%d] lag.%d\n",spentbp->hdrsi,bp->hdrsi,now - spentbp->lastprefetch);
                                    iguana_ramchain_prefetch(coin,&spentbp->ramchain,2);
                                    spentbp->lastprefetch = now;
                                }
                                spentU = (void *)(long)((long)spentbp->ramchain.H.data + spentbp->ramchain.H.data->Uoffset);
                                u = &spentU[spent_unspentind];
                                if ( (spent_pkind= u->pkind) != 0 && spent_pkind < spentbp->ramchain.H.data->numpkinds )
                                {
                                    memset(&ptr[emit],0,sizeof(ptr[emit]));
                                    if ( (ptr[emit].unspentind= spent_unspentind) != 0 && spentbp->hdrsi < bp->hdrsi )
                                    {
                                        ptr[emit].fromheight = bp->bundleheight + i;
                                        ptr[emit].hdrsi = spentbp->hdrsi;
                                        ptr[emit].pkind = spent_pkind;
                                        ptr[emit].value = u->value;
                                        //printf("ht.%d [%d] SPENDVECTOR u%d %.8f p%u\n",ptr[emit].fromheight,ptr[emit].hdrsi,ptr[emit].unspentind,dstr(ptr[emit].value),ptr[emit].pkind);
                                        //printf("(%d u%d).%d ",spentbp->hdrsi,unspentind,emit);
                                        emit++;
                                    }
                                    else
                                    {
                                        printf("spendvectors: null unspentind for spendind.%d hdrsi.%d [%d]\n",spendind,spentbp->hdrsi,bp->hdrsi);
                                        errs++;
                                        break;
                                    }
                                }
                                else
                                {
                                    errs++;
                                    printf("spendvectors: unresolved spendind.%d hdrsi.%d\n",spendind,bp->hdrsi);
                                    break;
                                }
                            }
                            else
                            {
                                memset(&ptr[emit],0,sizeof(ptr[emit]));
                                ptr[emit].hdrsi = spentbp->hdrsi;
                                ptr[emit].unspentind = spent_unspentind;
                                ptr[emit].fromheight = bp->bundleheight + i;
                                ptr[emit].tmpflag = 1;
                                if ( 0 && bp == coin->current )
                                    printf("fromht.%d spends [%d] TMPVECTOR u%d s%u\n",ptr[emit].fromheight,ptr[emit].hdrsi,ptr[emit].unspentind,spendind);
                                emit++;
                            }
                        }
                        else
                        {
                            errs++;
                            printf("spendvectors: error resolving external spendind.%d hdrsi.%d\n",spendind,bp->hdrsi);
                            break;
                        }
                    }
                }
            }
        }
        /*if ( iterate != 0 )
        {
            free(coin->fast[iter]);
            coin->fast[iter] = fastfind;
        }*/
        if ( txidind != ramchain->H.data->numtxids && txidind != ramchain->H.txidind )
        {
            printf("spendvectors: numtxid.%d != bp numtxids %d/%d\n",txidind,ramchain->H.txidind,ramchain->H.data->numtxids);
            errs++;
        }
        if ( spendind != ramchain->H.data->numspends && spendind != ramchain->H.spendind )
        {
            printf("spendvectors: spendind.%d != bp numspends %d/%d\n",spendind,ramchain->H.spendind,ramchain->H.data->numspends);
            errs++;
        }
    }
    if ( errs == 0 && emit >= 0 )
    {
        emitted += emit;
        if ( convertflag == 0 )
        {
            if ( bp->tmpspends != 0 )
            {
                if ( bp->tmpspends != ramchain->Xspendinds && emit > 0 )
                {
                    // printf("spendvectors: RT [%d] numtmpspends.%d vs starti.%d emit.%d\n",bp->hdrsi,bp->numtmpspends,starti,emit);
                    bp->tmpspends = myrealloc('x',bp->tmpspends,sizeof(*ptr)*bp->numtmpspends,sizeof(*ptr)*(bp->numtmpspends+emit));
                    memcpy(&bp->tmpspends[bp->numtmpspends],ptr,sizeof(*ptr)*emit);
                    bp->numtmpspends += emit;
                }
            }
            else if ( emit > 0 )
            {
                bp->tmpspends = myrealloc('x',ptr,sizeof(*ptr)*n,sizeof(*ptr)*emit);
                bp->numtmpspends = emit;
                //printf("ALLOC tmpspends.[%d]\n",bp->hdrsi);
                ptr = 0;
            }
            if ( 0 && bp == coin->current )
                printf("spendvectors.[%d]: tmpspends.%p[%d] after += emit.%d X.%p\n",bp->hdrsi,bp->tmpspends,bp->numtmpspends,emit,bp->ramchain.Xspendinds);
        } else errs = -iguana_spendvectorsave(coin,bp,ramchain,ptr!=0?ptr:bp->tmpspends,emit,n);
    }
    if ( ptr != 0 )
        myfree(ptr,sizeof(*ptr) * n);
    //if ( bp != coin->current )
        printf("UTXO [%4d].%-6d dur.%-2d [milli %8.3f] vectors %-6d err.%d [%5.2f%%] %7d %9s of %d\n",bp->hdrsi,bp->numtmpspends,(uint32_t)time(NULL)-starttime,OS_milliseconds()-startmillis,spendind,errs,100.*(double)emitted/(total+1),emit,mbstr(str,sizeof(*ptr) * emit),n);
    return(-errs);
}

int32_t iguana_balancegen(struct iguana_info *coin,int32_t incremental,struct iguana_bundle *bp,int32_t starti,int32_t endheight,int32_t startemit)
{
    uint32_t spent_unspentind,spent_pkind,txidind,h,i,j,endi,k,now; uint64_t spent_value;
    struct iguana_ramchain *ramchain; struct iguana_ramchaindata *rdata;
    struct iguana_spendvector *spend; struct iguana_unspent *spentU,*u; struct iguana_spendvector *Xspendinds;
    struct iguana_txid *T; struct iguana_blockRO *B; struct iguana_bundle *spentbp;
    int32_t spent_hdrsi,spendind,n,numXspends,errs=0,emit=0; struct iguana_spend *S,*s;
    ramchain = &bp->ramchain; //(bp == coin->current) ? &coin->RTramchain : &bp->ramchain;
    if ( (rdata= ramchain->H.data) == 0 || (n= ramchain->H.data->numspends) < 1 )
        return(-1);
    S = (void *)(long)((long)rdata + rdata->Soffset);
    B = (void *)(long)((long)rdata + rdata->Boffset);
    T = (void *)(long)((long)rdata + rdata->Toffset);
    numXspends = ramchain->numXspends;
    if ( (Xspendinds= ramchain->Xspendinds) == 0 )
    {
        numXspends = bp->numtmpspends;
        if ( (Xspendinds= bp->tmpspends) == 0 )
        {
            //printf("iguana_balancegen.%d: no Xspendinds[%d]\n",bp->hdrsi,numXspends);
            //return(-1);
        }
    }
    endi = (endheight % bp->n);
    txidind = B[starti].firsttxidind;
    spendind = B[starti].firstvin;
    emit = startemit;
    if ( coin->RTheight == 0 || bp->bundleheight+bp->n < coin->RTheight )
        fprintf(stderr,"BALANCEGEN.[%d] %p[%d] starti.%d s%d <-> endi.%d s%d startemit.%d\n",bp->hdrsi,Xspendinds,numXspends,starti,spendind,endi,B[endi].firstvin+B[endi].numvins,startemit);
    for (i=starti; i<=endi; i++)
    {
        now = (uint32_t)time(NULL);
        if ( 0 && bp == coin->current )
            printf("hdrs.[%d] B[%d] 1st txidind.%d txn_count.%d firstvin.%d firstvout.%d\n",bp->hdrsi,i,B[i].firsttxidind,B[i].txn_count,B[i].firstvin,B[i].firstvout);
        if ( txidind != B[i].firsttxidind || spendind != B[i].firstvin )
        {
            printf("balancegen: txidind %u != %u B[%d].firsttxidind || spendind %u != %u B[%d].firstvin errs.%d\n",txidind,B[i].firsttxidind,i,spendind,B[i].firstvin,i,errs);
            return(-1);
        }
        for (j=0; j<B[i].txn_count && errs==0; j++,txidind++)
        {
            now = (uint32_t)time(NULL);
            if ( txidind != T[txidind].txidind || spendind != T[txidind].firstvin )
            {
                printf("balancegen: txidind %u != %u T[txidind].firsttxidind || spendind %u != %u T[txidind].firstvin errs.%d\n",txidind,T[txidind].txidind,spendind,T[txidind].firstvin,errs);
                return(-1);
            }
            if ( 0 && bp == coin->current )
                printf("starti.%d txidind.%d txi.%d numvins.%d spendind.%d\n",i,txidind,j,T[txidind].numvins,spendind);
            for (k=0; k<T[txidind].numvins && errs==0; k++,spendind++)
            {
                s = &S[spendind];
                h = spent_hdrsi = -1;
                spent_value = 0;
                spent_unspentind = spent_pkind = 0;
                if ( s->external != 0 && s->prevout >= 0 )
                {
                    if ( emit >= numXspends )
                        errs++;
                    else if ( Xspendinds != 0 )
                    {
                        spend = &Xspendinds[emit++];
                        spent_unspentind = spend->unspentind;
                        spent_value = spend->value;
                        spent_pkind = spend->pkind;
                        spent_hdrsi = spend->hdrsi;
                        h = spend->fromheight;
                    }
                    if ( 0 && bp == coin->current )
                        printf("external prevout.%d (emit.%d numX.%d) %p u%d p%d errs.%d spent_hdrsi.%d s%u\n",s->prevout,emit,numXspends,Xspendinds,spent_unspentind,spent_pkind,errs,spent_hdrsi,spendind);
                }
                else if ( s->prevout >= 0 )
                {
                    h = bp->bundleheight + i;
                    spent_hdrsi = bp->hdrsi;
                    if ( s->spendtxidind != 0 && s->spendtxidind < rdata->numtxids )
                    {
                        spent_unspentind = T[s->spendtxidind].firstvout + s->prevout;
                        spentU = (void *)(long)((long)rdata + rdata->Uoffset);
                        u = &spentU[spent_unspentind];
                        if ( (spent_pkind= u->pkind) != 0 && spent_pkind < rdata->numpkinds )
                            spent_value = u->value;
                        /*found spend d9151... txidind.1083097 [202] s3163977
                        //found spend d9151... txidind.1083097 [202] s4033628
                        if ( spent_hdrsi == 202 && (spendind == 3163977 || spendind == 4033628) )
                            printf("internal spend.%d spendtxidind.%d 1st.%d U.(prevout.%d u%u pkind.%u %.8f)\n",spendind,txidind,T[s->spendtxidind].firstvout,s->prevout,spent_unspentind,u->pkind,dstr(u->value));*/
                    }
                    else //if ( i > 0 || j > 0 || k > 0 )
                    {
                        printf("iguana_balancegen [%d] txidind overflow %u vs %u (%d %d %d)\n",bp->hdrsi,s->spendtxidind,rdata->numtxids,i,j,k);
                        errs++;
                    }
                }
                else continue;
                spentbp = 0;
                if ( (spentbp= coin->bundles[spent_hdrsi]) != 0 && spent_unspentind > 0 && spent_pkind > 0 )
                {
                    if ( 0 && bp == coin->current )
                        printf("[%d] spendind.%u -> [%d] u%d\n",bp->hdrsi,spendind,spent_hdrsi,spent_unspentind);
                    if ( iguana_volatileupdate(coin,incremental,spentbp == coin->current ? &coin->RTramchain : &spentbp->ramchain,spent_hdrsi,spent_unspentind,spent_pkind,spent_value,spendind,h) < 0 )
                        errs++;
                }
                else //if ( Xspendinds != 0 )
                {
                    errs++;
                    printf("iguana_balancegen: spendind.%u external.%d error spentbp.%p with unspentind.%d pkind.%u [%d] (%d %d %d)\n",spendind,s->external,spentbp,spent_unspentind,spent_pkind,spent_hdrsi,i,j,k);
                }
            }
        }
    }
    if ( txidind != bp->ramchain.H.data->numtxids && (bp != coin->current || txidind != ramchain->H.txidind) )
    {
        printf("numtxid.%d != bp numtxids %d/%d\n",txidind,bp->ramchain.H.txidind,bp->ramchain.H.data->numtxids);
        errs++;
    }
    if ( spendind != ramchain->H.data->numspends && (bp != coin->current || spendind != ramchain->H.spendind) )
    {
        printf("spendind.%d != bp numspends %d/%d\n",spendind,bp->ramchain.H.spendind,bp->ramchain.H.data->numspends);
        errs++;
    }
    if ( emit != numXspends )
    {
        printf("iguana_balancegen: emit %d != %d ramchain->numXspends\n",emit,numXspends);
        errs++;
    }
    if ( errs == 0 )
        bp->balancefinish = (uint32_t)time(NULL);
    //printf(">>>>>>>> balances.%d done errs.%d spendind.%d\n",bp->hdrsi,errs,n);
    return(-errs);
}

void iguana_truncatebalances(struct iguana_info *coin)
{
    int32_t i; struct iguana_bundle *bp;
    for (i=0; i<coin->balanceswritten; i++)
    {
        if ( (bp= coin->bundles[i]) != 0 )
        {
            bp->balancefinish = 0;
            bp->Xvalid = 0;
            iguana_volatilespurge(coin,&bp->ramchain);
        }
    }
    coin->balanceswritten = 0;
}

int32_t iguana_volatilesinit(struct iguana_info *coin)
{
    bits256 balancehash,allbundles; struct iguana_utxo *Uptr; struct iguana_account *Aptr;
    struct sha256_vstate vstate,bstate; int32_t i,from_ro,numpkinds,numunspents; struct iguana_bundle *bp; struct iguana_block *block;
    uint32_t crc,filecrc; FILE *fp; char crcfname[512],str[65],str2[65],buf[2048];
    from_ro = 1;
    for (i=0; i<coin->balanceswritten; i++)
    {
        if ( (bp= coin->bundles[i]) == 0 )
            break;
        if ( bp->emitfinish <= 1 || (i > 0 && bp->utxofinish <= 1) )
        {
            printf("hdrsi.[%d] emitfinish.%u utxofinish.%u\n",i,bp->emitfinish,bp->utxofinish);
            break;
        }
        iguana_volatilesmap(coin,&bp->ramchain);
        if ( from_ro != 0 && (bp->ramchain.from_ro == 0 || (bp->hdrsi > 0 && bp->ramchain.from_roX == 0) || bp->ramchain.from_roA == 0 || bp->ramchain.from_roU == 0) )
        {
            printf("from_ro.[%d] %d %d %d %d\n",bp->hdrsi,bp->ramchain.from_ro,bp->ramchain.from_roX,bp->ramchain.from_roA,bp->ramchain.from_roU);
            from_ro = 0;
        }
    }
    if ( i < coin->balanceswritten )
    {
        printf("TRUNCATE balances written.%d -> %d\n",coin->balanceswritten,i);
        iguana_truncatebalances(coin);
    }
    else
    {
        //printf("verify crc and sha256 hash for %d of %d\n",i,coin->balanceswritten);
        vupdate_sha256(balancehash.bytes,&vstate,0,0);
        vupdate_sha256(allbundles.bytes,&bstate,0,0);
        filecrc = 0;
        sprintf(crcfname,"%s/%s/balancecrc.%d",GLOBAL_DBDIR,coin->symbol,coin->balanceswritten);
        if ( (fp= fopen(crcfname,"rb")) != 0 )
        {
            if ( fread(&filecrc,1,sizeof(filecrc),fp) != sizeof(filecrc) )
                filecrc = 0;
            else if ( fread(&balancehash,1,sizeof(balancehash),fp) != sizeof(balancehash) )
                filecrc = 0;
            else if ( memcmp(&balancehash,&coin->balancehash,sizeof(balancehash)) != 0 )
                filecrc = 0;
            else if ( fread(&allbundles,1,sizeof(allbundles),fp) != sizeof(allbundles) )
                filecrc = 0;
            else if ( memcmp(&allbundles,&coin->allbundles,sizeof(allbundles)) != 0 )
                filecrc = 0;
            fclose(fp);
        }
        if ( filecrc != 0 )
            printf("have filecrc.%08x for %s milli.%.0f from_ro.%d\n",filecrc,bits256_str(str,balancehash),OS_milliseconds(),from_ro);
        if ( from_ro == 0 || filecrc == 0 )
        {
            if ( filecrc == 0 )
            {
                vupdate_sha256(balancehash.bytes,&vstate,0,0);
                vupdate_sha256(allbundles.bytes,&bstate,0,0);
            }
            for (i=crc=0; i<coin->balanceswritten; i++)
            {
                numpkinds = numunspents = 0;
                Aptr = 0, Uptr = 0;
                if ( (bp= coin->bundles[i]) != 0 && bp->ramchain.H.data != 0 && (numpkinds= bp->ramchain.H.data->numpkinds) > 0 && (numunspents= bp->ramchain.H.data->numunspents) > 0 && (Aptr= bp->ramchain.A2) != 0 && (Uptr= bp->ramchain.Uextras) != 0 )
                {
                    if ( (bp->bundleheight % 10000) == 0 )
                        fprintf(stderr,".");
                    if ( filecrc == 0 )
                    {
                        vupdate_sha256(balancehash.bytes,&vstate,(void *)Aptr,sizeof(*Aptr) * numpkinds);
                        vupdate_sha256(balancehash.bytes,&vstate,(void *)Uptr,sizeof(*Uptr) * numunspents);
                        vupdate_sha256(allbundles.bytes,&bstate,(void *)bp->hashes,sizeof(bp->hashes[0]) * bp->n);
                    }
                    crc = calc_crc32(crc,(void *)Aptr,(int32_t)(sizeof(*Aptr) * numpkinds));
                    crc = calc_crc32(crc,(void *)Uptr,(int32_t)(sizeof(*Uptr) * numunspents));
                    crc = calc_crc32(crc,(void *)bp->hashes,(int32_t)(sizeof(bp->hashes[0]) * bp->n));
                } //else printf("missing hdrs.[%d] data.%p num.(%u %d) %p %p\n",i,bp->ramchain.H.data,numpkinds,numunspents,Aptr,Uptr);
            }
        } else crc = filecrc;
        printf("millis %.0f from_ro.%d written.%d crc.%08x/%08x balancehash.(%s) vs (%s)\n",OS_milliseconds(),from_ro,coin->balanceswritten,crc,filecrc,bits256_str(str,balancehash),bits256_str(str2,coin->balancehash));
        if ( (filecrc != 0 && filecrc != crc) || memcmp(balancehash.bytes,coin->balancehash.bytes,sizeof(balancehash)) != 0 || memcmp(allbundles.bytes,coin->allbundles.bytes,sizeof(allbundles)) != 0 )
        {
            printf("balancehash or crc.(%x %x) mismatch or allbundles.(%llx %llx) mismatch\n",crc,filecrc,(long long)allbundles.txid,(long long)coin->allbundles.txid);
            iguana_truncatebalances(coin);
            OS_removefile(crcfname,0);
        }
        else
        {
            printf("MATCHED balancehash numhdrsi.%d crc.%08x\n",coin->balanceswritten,crc);
            if ( (fp= fopen(crcfname,"wb")) != 0 )
            {
                if ( fwrite(&crc,1,sizeof(crc),fp) != sizeof(crc) || fwrite(&balancehash,1,sizeof(balancehash),fp) != sizeof(balancehash) || fwrite(&allbundles,1,sizeof(allbundles),fp) != sizeof(allbundles) )
                    printf("error writing.(%s)\n",crcfname);
                fclose(fp);
            }
            else
            {
                printf("volatileinit: cant create.(%s)\n",crcfname);
                return(-1);
            }
        }
    }
    if ( (coin->RTheight= coin->balanceswritten * coin->chain->bundlesize) > coin->longestchain )
        coin->longestchain = coin->RTheight;
    iguana_bundlestats(coin,buf,IGUANA_DEFAULTLAG);
    if ( (bp= coin->bundles[coin->balanceswritten-1]) != 0 && (block= bp->blocks[bp->n-1]) != 0 )
    {
        //char str[65];
        //printf("set hwmchain.%d <- %s %p\n",bp->bundleheight+bp->n-1,bits256_str(str,bp->hashes[bp->n-1]),block);
        if ( block->height > coin->blocks.hwmchain.height )
            coin->blocks.hwmchain = *block;
    }
    //printf("end volatilesinit\n");
    return(coin->balanceswritten);
}

void iguana_initfinal(struct iguana_info *coin,bits256 lastbundle)
{
    int32_t i; struct iguana_bundle *bp; bits256 hash2; struct iguana_block *block; char hashstr[65];
    if ( bits256_nonz(lastbundle) > 0 )
    {
        init_hexbytes_noT(hashstr,lastbundle.bytes,sizeof(bits256));
        printf("req lastbundle.(%s)\n",hashstr);
        queue_enqueue("hdrsQ",&coin->hdrsQ,queueitem(hashstr),1);
    }
    for (i=0; i<coin->bundlescount-1; i++)
    {
        if ( (bp= coin->bundles[i]) == 0 || bp->emitfinish <= 1 )
        {
            printf("initfinal break.[%d]: bp.%p or emit.%u utxofinish.%u\n",i,bp,bp!=0?bp->emitfinish:-1,bp!=0?bp->utxofinish:-1);
            break;
        }
        if ( i == 0 )
            bp->utxofinish = bp->startutxo = (uint32_t)time(NULL);
    }
    if ( i < coin->bundlescount-1 )
    {
        printf("spendvectors.[%d] max.%d missing, will regen all of them\n",i,coin->bundlescount-1);
        for (i=0; i<coin->bundlescount-1; i++)
        {
            if ( (bp= coin->bundles[i]) != 0 )
                bp->startutxo = bp->utxofinish = 0;
        }
    }
    else
    {
        for (i=0; i<coin->bundlescount-1; i++)
        {
            if ( (bp= coin->bundles[i]) != 0 )
                bp->converted = (uint32_t)time(NULL);
        }
    }
    printf("i.%d bundlescount.%d\n",i,coin->bundlescount);
    if ( coin->balanceswritten > 1 )
        coin->balanceswritten = iguana_volatilesinit(coin);
    if ( coin->balanceswritten > 1 )
    {
        for (i=0; i<coin->balanceswritten; i++)
        {
            //printf("%d ",i);
            iguana_validateQ(coin,coin->bundles[i]);
        }
    }
    printf("i.%d balanceswritten.%d\n",i,coin->balanceswritten);
    if ( coin->balanceswritten < coin->bundlescount )
    {
        for (i=coin->balanceswritten; i<coin->bundlescount; i++)
        {
            if ( (bp= coin->bundles[i]) != 0 && bp->queued == 0 )
            {
                //printf("%d ",i);
                iguana_bundleQ(coin,bp,1000);
            }
        }
        printf("iguana_bundlesQ %d to %d\n",coin->balanceswritten,coin->bundlescount);
    }
    if ( (coin->origbalanceswritten= coin->balanceswritten) > 0 )
        iguana_volatilesinit(coin);
    iguana_savehdrs(coin);
    iguana_fastlink(coin,coin->balanceswritten * coin->chain->bundlesize - 1);
    iguana_walkchain(coin,0);
    hash2 = iguana_blockhash(coin,coin->balanceswritten * coin->chain->bundlesize);
    if ( bits256_nonz(hash2) != 0 && (block= iguana_blockfind("initfinal",coin,hash2)) != 0 )
        _iguana_chainlink(coin,block);
    if ( iguana_fastfindinit(coin) == 0 && coin->PREFETCHLAG >= 0 )
        iguana_fastfindcreate(coin);
}

int32_t iguana_balanceflush(struct iguana_info *coin,int32_t refhdrsi)
{
    int32_t hdrsi,numpkinds,iter,numhdrsi,i,numunspents,err; struct iguana_bundle *bp;
    char fname[1024],fname2[1024],destfname[1024]; bits256 balancehash,allbundles; FILE *fp,*fp2;
    struct iguana_utxo *Uptr; struct iguana_account *Aptr; struct sha256_vstate vstate,bstate;
    vupdate_sha256(balancehash.bytes,&vstate,0,0);
    numhdrsi = refhdrsi;
    vupdate_sha256(balancehash.bytes,&vstate,0,0);
    vupdate_sha256(allbundles.bytes,&bstate,0,0);
    for (iter=0; iter<3; iter++)
    {
        for (hdrsi=0; hdrsi<numhdrsi; hdrsi++)
        {
            Aptr = 0;
            Uptr = 0;
            numunspents = numpkinds = 0;
            if ( (bp= coin->bundles[hdrsi]) != 0 && bp->ramchain.H.data != 0 && (numpkinds= bp->ramchain.H.data->numpkinds) > 0 && (numunspents= bp->ramchain.H.data->numunspents) > 0 && (Aptr= bp->ramchain.A2) != 0 && (Uptr= bp->ramchain.Uextras) != 0 )
            {
                sprintf(fname,"%s/%s/debits.%d_N%d",GLOBAL_TMPDIR,coin->symbol,bp->hdrsi,numhdrsi);
                sprintf(fname2,"%s/%s/lastspends.%d_N%d",GLOBAL_TMPDIR,coin->symbol,bp->hdrsi,numhdrsi);
                if ( iter == 0 )
                {
                    vupdate_sha256(balancehash.bytes,&vstate,(void *)Aptr,sizeof(*Aptr)*numpkinds);
                    vupdate_sha256(balancehash.bytes,&vstate,(void *)Uptr,sizeof(*Uptr)*numunspents);
                    vupdate_sha256(allbundles.bytes,&bstate,(void *)bp->hashes,sizeof(bp->hashes[0])*bp->n);
                }
                else if ( iter == 1 )
                {
                    if ( (fp= fopen(fname,"wb")) != 0 && (fp2= fopen(fname2,"wb")) != 0 )
                    {
                        err = -1;
                        if ( fwrite(&numhdrsi,1,sizeof(numhdrsi),fp) == sizeof(numhdrsi) && fwrite(&numhdrsi,1,sizeof(numhdrsi),fp2) == sizeof(numhdrsi) && fwrite(balancehash.bytes,1,sizeof(balancehash),fp) == sizeof(balancehash) && fwrite(balancehash.bytes,1,sizeof(balancehash),fp2) == sizeof(balancehash) && fwrite(allbundles.bytes,1,sizeof(allbundles),fp) == sizeof(allbundles) && fwrite(allbundles.bytes,1,sizeof(allbundles),fp2) == sizeof(allbundles) )
                        {
                            if ( fwrite(Aptr,sizeof(*Aptr),numpkinds,fp) == numpkinds )
                            {
                                if ( fwrite(Uptr,sizeof(*Uptr),numunspents,fp2) == numunspents )
                                {
                                    err = 0;
                                    printf("[%d] of %d saved (%s) and (%s)\n",hdrsi,numhdrsi,fname,fname2);
                                }
                            }
                        }
                        if ( err != 0 )
                        {
                            printf("balanceflush.%s error iter.%d hdrsi.%d\n",coin->symbol,iter,hdrsi);
                            fclose(fp);
                            fclose(fp2);
                            return(-1);
                        }
                        fclose(fp), fclose(fp2);
                    }
                    else
                    {
                        printf("error opening %s or %s %p\n",fname,fname2,fp);
                        if ( fp != 0 )
                            fclose(fp);
                    }
                }
                else if ( iter == 2 )
                {
                    sprintf(destfname,"%s/%s/accounts/debits.%d",GLOBAL_DBDIR,coin->symbol,bp->bundleheight);
                    if ( OS_copyfile(fname,destfname,1) < 0 )
                    {
                        printf("balances error copying (%s) -> (%s)\n",fname,destfname);
                        return(-1);
                    }
                    sprintf(destfname,"%s/%s/accounts/lastspends.%d",GLOBAL_DBDIR,coin->symbol,bp->bundleheight);
                    if ( OS_copyfile(fname2,destfname,1) < 0 )
                    {
                        printf("balances error copying (%s) -> (%s)\n",fname2,destfname);
                        return(-1);
                    }
                    printf("%s -> %s\n",fname,destfname);
                    OS_removefile(fname,0);
                    OS_removefile(fname2,0);
                }
                if ( bp->ramchain.allocatedA2 == 0 || bp->ramchain.allocatedU2 == 0 )
                {
                    printf("skip saving.[%d] files as not allocated\n",bp->hdrsi);
                    break;
                }
            }
            else if ( hdrsi > 0 )
            {
                printf("balanceflush iter.%d error loading [%d] Aptr.%p Uptr.%p numpkinds.%u numunspents.%u\n",iter,hdrsi,Aptr,Uptr,numpkinds,numunspents);
                return(-1);
            }
        }
    }
    coin->allbundles = allbundles;
    coin->balancehash = balancehash;
    coin->balanceswritten = numhdrsi;
    if ( 1 )
    {
        for (hdrsi=0; hdrsi<numhdrsi; hdrsi++)
            if ( (bp= coin->bundles[hdrsi]) == 0 )
            {
                iguana_volatilespurge(coin,&bp->ramchain);
                if ( iguana_volatilesmap(coin,&bp->ramchain) != 0 )
                    printf("error mapping bundle.[%d]\n",hdrsi);
            }
    }
    char str[65]; printf("BALANCES WRITTEN for %d orig.%d bundles %s\n",coin->balanceswritten,coin->origbalanceswritten,bits256_str(str,coin->balancehash));
    if ( 0 && coin->balanceswritten > coin->origbalanceswritten+10 ) // strcmp(coin->symbol,"BTC") == 0 &&
    {
        coin->active = 0;
        coin->started = 0;
        for (i=0; i<IGUANA_MAXPEERS; i++)
            coin->peers.active[i].dead = (uint32_t)time(NULL);
#ifdef __linux__
        char cmd[1024];
        sprintf(cmd,"mksquashfs %s/%s %s.%d -comp xz",GLOBAL_DBDIR,coin->symbol,coin->symbol,coin->balanceswritten);
        if ( system(cmd) != 0 )
            printf("error system(%s)\n",cmd);
        else
        {
            sprintf(cmd,"sudo umount %s/ro/%s",GLOBAL_DBDIR,coin->symbol);
            if ( system(cmd) != 0 )
                printf("error system(%s)\n",cmd);
            else
            {
                sprintf(cmd,"sudo mount %s.%d %s/ro/%s -t squashfs -o loop",coin->symbol,coin->balanceswritten,GLOBAL_DBDIR,coin->symbol);
                if ( system(cmd) != 0 )
                    printf("error system(%s)\n",cmd);
            }
        }
#endif
        for (i=0; i<30; i++)
        {
            printf("need to exit, please restart after shutdown in %d seconds, or just ctrl-C\n",30-i);
            sleep(1);
        }
        exit(-1);
    }
    coin->balanceswritten = iguana_volatilesinit(coin);
    //printf("flush free\n");
    iguana_RTramchainfree(coin,bp);
    return(coin->balanceswritten);
}

int32_t iguana_spendvectorsaves(struct iguana_info *coin)
{
    int32_t i,j,n,iter; struct iguana_bundle *bp;
    if ( coin->spendvectorsaved > 1 )
        return(0);
    coin->spendvectorsaved = 1;
    n = coin->bundlescount - 1;
    //printf("SAVE SPEND VECTORS %d of %d\n",n,coin->bundlescount);
    for (iter=0; iter<2; iter++)
    {
        for (i=0; i<n; i++)
        {
            if ( (bp= coin->bundles[i]) != 0 )
            {
                if ( iter == 0 )
                {
                    if ( bp->tmpspends != 0 )//bp->ramchain.Xspendinds == 0 &&
                    {
                        for (j=0; j<bp->numtmpspends; j++)
                            if ( bp->tmpspends[j].tmpflag != 0 )
                            {
                                printf("vectorsave.[%d] vec.%d still has tmpflag\n",i,j);
                                return(-1);
                            }
                    }
                }
                else if ( iguana_spendvectorsave(coin,bp,&bp->ramchain,bp->tmpspends,bp->numtmpspends,bp->ramchain.H.data->numspends) == 0 )
                {
                    if ( bp->tmpspends != 0 && bp->numtmpspends > 0 && bp->tmpspends != bp->ramchain.Xspendinds )
                        myfree(bp->tmpspends,sizeof(*bp->tmpspends) * bp->numtmpspends);
                    bp->numtmpspends = 0;
                    bp->tmpspends = 0;
                }
            }
        }
    }
    coin->spendvectorsaved = (uint32_t)time(NULL);
    return(0);
}

int32_t iguana_spendvectorconvs(struct iguana_info *coin,struct iguana_bundle *spentbp,int32_t starti)
{
    struct iguana_bundle *bp; int16_t spent_hdrsi; uint32_t numpkinds; struct iguana_unspent *spentU; struct iguana_spendvector *vec; int32_t i,converted,j,n = coin->bundlescount; struct iguana_ramchain *ramchain; struct iguana_ramchaindata *rdata = 0;
    if ( (rdata= spentbp->ramchain.H.data) == 0 )
    {
        //if ( spentbp == coin->current )
            printf("iguana_spendvectorconvs: [%d] null rdata.%p\n",spentbp->hdrsi,rdata);
        return(-1);
    }
    spent_hdrsi = spentbp->hdrsi;
    ramchain = &spentbp->ramchain;
    numpkinds = rdata->numpkinds;
    spentU = (void *)(long)((long)rdata + rdata->Uoffset);
    for (i=converted=0; i<n; i++)
    {
        if ( (bp= coin->bundles[i]) != 0 && bp->tmpspends != 0 )
        {
            for (j=0; j<bp->numtmpspends; j++)
            {
                vec = &bp->tmpspends[j];
                if ( vec->hdrsi == spent_hdrsi )
                {
                    if ( vec->tmpflag == 0 )
                    {
                        if ( bp->tmpspends != bp->ramchain.Xspendinds && bp != coin->current )
                            printf("unexpected null tmpflag [%d] j.%d spentbp.[%d]\n",bp->hdrsi,j,spentbp->hdrsi);
                    }
                    else
                    {
                        if ( _iguana_spendvectorconv(vec,&spentU[vec->unspentind],numpkinds,vec->hdrsi,vec->unspentind) != 0 )
                            converted++;
                        else
                        {
                            printf("iguana_spendvectorconv.[%d] error [%d] at %d of T[%d/%d] [%d] u%u p%u\n",spentbp->hdrsi,bp->hdrsi,j,bp->numtmpspends,n,vec->hdrsi,vec->unspentind,spentU[vec->unspentind].pkind);
                            return(-1);
                        }
                    }
                }
            }
        }
        else if ( bp->hdrsi < coin->bundlescount-1 )
        {
            //printf("iguana_spendvectorconvs: [%d] null bp.%p\n",i,bp);
        }
    }
    spentbp->converted = (uint32_t)time(NULL);
    //printf("spendvectorconvs.[%d] converted.%d\n",refbp->hdrsi,converted);
    return(converted);
}

int32_t iguana_convert(struct iguana_info *coin,int32_t helperid,struct iguana_bundle *bp,int32_t RTflag,int32_t starti)
{
    static int64_t total[256],depth;
    int32_t i,n,m,max,converted; int64_t total_tmpspends,sum; double startmillis = OS_milliseconds();
    depth++;
    if ( (converted= iguana_spendvectorconvs(coin,bp,starti)) < 0 )
    {
        printf("error iguana_convert.[%d]\n",bp->hdrsi);
        return(0);
    }
    else
    {
        n = coin->bundlescount;
        for (i=m=total_tmpspends=0; i<n; i++)
        {
            if ( coin->bundles[i] != 0 )
            {
                total_tmpspends += coin->bundles[i]->numtmpspends;
                if ( coin->bundles[i]->converted > 1 )
                    m++;
            }
        }
        max = (int32_t)(sizeof(total) / sizeof(*total));
        total[helperid % max] += converted;
        for (i=sum=0; i<max; i++)
            sum += total[i];
        if ( converted != 0 )
            printf("[%4d] millis %7.3f converted.%-7d balance calc.%-4d of %4d | total.%llu of %llu depth.%d\n",bp->hdrsi,OS_milliseconds()-startmillis,converted,m,n,(long long)sum,(long long)total_tmpspends,(int32_t)depth);
    }
    depth--;
    return(converted);
}

void iguana_RTramchainfree(struct iguana_info *coin,struct iguana_bundle *bp)
{
    //printf("free RTramchain\n");
    iguana_utxoupdate(coin,-1,0,0,0,0,-1); // free hashtables
    coin->RTheight = coin->balanceswritten * coin->chain->bundlesize;
    coin->RTgenesis = 0;
    iguana_ramchain_free(coin,&coin->RTramchain,1);
    if ( bp != 0 )
        bp->ramchain = coin->RTramchain;
    iguana_mempurge(&coin->RTmem);
    iguana_mempurge(&coin->RThashmem);
    coin->RTdatabad = 0;
}

void *iguana_ramchainfile(struct iguana_info *coin,struct iguana_ramchain *dest,struct iguana_ramchain *R,struct iguana_bundle *bp,int32_t bundlei,struct iguana_block *block)
{
    char fname[1024]; long filesize; int32_t err; void *ptr=0;
    if ( block == bp->blocks[bundlei] && (ptr= iguana_bundlefile(coin,fname,&filesize,bp,bundlei)) != 0 )
    {
        if ( iguana_mapchaininit(fname,coin,R,bp,bundlei,block,ptr,filesize) >= 0 )
        {
            if ( dest != 0 && dest->H.data != 0 )
                err = iguana_ramchain_iterate(coin,dest,R,bp,bundlei);
            else err = 0;
            if ( err != 0 || dest->H.data == 0 || bits256_cmp(R->H.data->firsthash2,block->RO.hash2) != 0 )
            {
                char str[65];
                printf("ERROR [%d:%d] %s vs ",bp->hdrsi,bundlei,bits256_str(str,block->RO.hash2));
                printf("mapped.%s\n",bits256_str(str,R->H.data->firsthash2));
            } else return(ptr);
        }
        iguana_blockunmark(coin,block,bp,bundlei,1);
        iguana_ramchain_free(coin,R,1);
    } //else printf("ramchainfile ptr.%p block.%p\n",ptr,block);
    return(0);
}

void iguana_RTramchainalloc(char *fname,struct iguana_info *coin,struct iguana_bundle *bp)
{
    uint32_t i,changed = 0; struct iguana_ramchaindata *rdata; struct iguana_ramchain *dest = &coin->RTramchain; struct iguana_blockRO *B; struct iguana_bundle *tmpbp;
    if ( (rdata= dest->H.data) != 0 )
    {
        i = 0;
        if ( coin->RTheight != bp->bundleheight + rdata->numblocks )
            changed++;
        else
        {
            B = (void *)(long)((long)rdata + rdata->Boffset);
            for (i=0; i<rdata->numblocks; i++)
                if ( bits256_cmp(B[i].hash2,bp->hashes[i]) != 0 )
                {
                    char str[65],str2[65]; printf("mismatched hash2 at %d %s vs %s\n",bp->bundleheight+i,bits256_str(str,B[i].hash2),bits256_str(str2,bp->hashes[i]));
                    changed++;
                    break;
                }
        }
        if ( changed != 0 )
        {
            printf("RTramchain changed %d bundlei.%d | coin->RTheight %d != %d bp->bundleheight +  %d coin->RTramchain.H.data->numblocks\n",coin->RTheight,i,coin->RTheight,bp->bundleheight,rdata->numblocks);
            //coin->RTheight = coin->balanceswritten * coin->chain->bundlesize;
            iguana_RTramchainfree(coin,bp);
        }
    }
    if ( coin->RTramchain.H.data == 0 )
    {
        printf("ALLOC RTramchain\n");
        iguana_ramchainopen(fname,coin,dest,&coin->RTmem,&coin->RThashmem,bp->bundleheight,bp->hashes[0]);
        dest->H.txidind = dest->H.unspentind = dest->H.spendind = dest->pkind = dest->H.data->firsti;
        dest->externalind = dest->H.stacksize = 0;
        dest->H.scriptoffset = 1;
        if ( 1 )
        {
            for (i=0; i<bp->hdrsi; i++)
                if ( (tmpbp= coin->bundles[i]) != 0 )
                {
                    iguana_volatilespurge(coin,&tmpbp->ramchain);
                    iguana_volatilesmap(coin,&tmpbp->ramchain);
                }
            sleep(1);
        }
    }
}

void iguana_rdataset(struct iguana_ramchain *dest,struct iguana_ramchaindata *rdest,struct iguana_ramchain *src)
{
    *dest = *src;
    dest->H.data = rdest;
    *rdest = *src->H.data;
    rdest->numpkinds = src->pkind;
    rdest->numexternaltxids = src->externalind;
    rdest->numtxids = src->H.txidind;
    rdest->numunspents = src->H.unspentind;
    rdest->numspends = src->H.spendind;
    //printf("RT set numtxids.%u numspends.%u\n",rdest->numtxids,rdest->numspends);
}

void iguana_rdatarestore(struct iguana_ramchain *dest,struct iguana_ramchaindata *rdest,struct iguana_ramchain *src)
{
    *src = *dest;
    *src->H.data = *rdest;
    src->pkind = rdest->numpkinds;
    src->externalind = rdest->numexternaltxids;
    src->H.txidind = rdest->numtxids;
    src->H.unspentind = rdest->numunspents;
    src->H.spendind = rdest->numspends;
    printf("RT restore numtxids.%u numspends.%u\n",rdest->numtxids,rdest->numspends);
}

void iguana_RThdrs(struct iguana_info *coin,struct iguana_bundle *bp,int32_t numaddrs)
{
    int32_t datalen,i; uint8_t serialized[512]; char str[65]; struct iguana_peer *addr;
    for (i=0; i<numaddrs && i<coin->peers.numranked; i++)
    {
        queue_enqueue("hdrsQ",&coin->hdrsQ,queueitem(bits256_str(str,bp->hashes[0])),1);
        if ( (addr= coin->peers.ranked[i]) != 0 && addr->usock >= 0 && addr->dead == 0 && (datalen= iguana_gethdrs(coin,serialized,coin->chain->gethdrsmsg,bits256_str(str,bp->hashes[0]))) > 0 )
        {
            iguana_send(coin,addr,serialized,datalen);
            addr->pendhdrs++;
        }
    }
}

void iguana_RTspendvectors(struct iguana_info *coin,struct iguana_bundle *bp)
{
    int32_t iterate,lasti,num,hdrsi,orignumemit; struct iguana_ramchain R; struct iguana_ramchaindata RDATA;
    if ( bp->hdrsi <= 0 )
        return;
    bp->ramchain = coin->RTramchain;
    iguana_rdataset(&R,&RDATA,&coin->RTramchain);
    if ( (lasti= (coin->RTheight - ((coin->RTheight/bp->n)*bp->n))) >= bp->n-1 )
        lasti = bp->n - 1;
    orignumemit = bp->numtmpspends;
#ifdef __APPLE__
    iterate = 0*(coin->bundlescount-1);
#else
    iterate = 0;
#endif
    if ( iguana_spendvectors(coin,bp,&coin->RTramchain,coin->RTstarti,lasti,0,iterate) < 0 )
    {
        printf("RTutxo error -> RTramchainfree\n");
        coin->RTdatabad = 1;
        return;
    }
    else
    {
        printf("spendvectors calculated to %d [%d]\n",coin->RTheight,bp->hdrsi);
        bp->converted = 1;
        for (hdrsi=num=0; hdrsi<bp->hdrsi; hdrsi++)
        {
#ifdef __APPLE__
            iguana_ramchain_prefetch(coin,&coin->bundles[hdrsi]->ramchain,2);
#endif
            num += iguana_convert(coin,IGUANA_NUMHELPERS,coin->bundles[hdrsi],1,orignumemit);
        }
        printf("spendvectors converted.%d to %d\n",num,coin->RTheight);
        bp->converted = (uint32_t)time(NULL);
        if ( iguana_balancegen(coin,1,bp,coin->RTstarti,coin->RTheight > 0 ? coin->RTheight-1 : bp->n-1,orignumemit) < 0 )
            coin->RTdatabad = 1;
        //printf("iguana_balancegen [%d] (%d to %d)\n",bp->hdrsi,coin->RTstarti,(coin->RTheight-1)%bp->n);
        coin->RTstarti = (coin->RTheight % bp->n);
    }
}

int32_t iguana_realtime_update(struct iguana_info *coin)
{
    double startmillis0; static double totalmillis0; static int32_t num0;
    struct iguana_bundle *bp; struct iguana_ramchaindata *rdata; int32_t bundlei,i,n,flag=0; bits256 hash2; struct iguana_peer *addr;
    struct iguana_block *block=0; struct iguana_blockRO *B; struct iguana_ramchain *dest=0,blockR;
    if ( coin->current != 0 && (coin->blocks.hwmchain.height % coin->chain->bundlesize) == coin->chain->bundlesize-1 )
    {
        //printf("RT edge case\n");
    }
    if ( 0 && coin->RTheight > 0 && coin->spendvectorsaved != 1 && coin->bundlescount-1 != coin->balanceswritten )
    {
        printf("RT mismatch %d != %d\n",coin->bundlescount-1,coin->balanceswritten);
        coin->spendvectorsaved = 0;
        iguana_RTramchainfree(coin,coin->current);
        return(0);
    }
    if ( coin->spendvectorsaved <= 1 )
        return(0);
    for (i=0; i<coin->bundlescount-1; i++)
    {
        if ( (bp= coin->bundles[i]) != 0 && (i > 0 && bp->utxofinish == 0) )
        {
            if ( iguana_spendvectors(coin,bp,&bp->ramchain,0,bp->n,0,0) < 0 )
            {
                printf("error generating spendvectors.[%d], exiting. just restart iguana\n",i);
                exit(-1);
            } // else printf("generated UTXO.[%d]\n",i);
            coin->spendvectorsaved = 1;
        }
    }
    if ( coin->RTdatabad == 0 && (bp= coin->current) != 0 && bp->hdrsi == coin->longestchain/coin->chain->bundlesize && bp->hdrsi == coin->balanceswritten && coin->RTheight >= bp->bundleheight && coin->RTheight < bp->bundleheight+bp->n && ((coin->RTheight <= coin->blocks.hwmchain.height && time(NULL) > bp->lastRT) || time(NULL) > bp->lastRT+10) )
    {
        if ( (block= bp->blocks[0]) == 0 || block->txvalid == 0 || block->mainchain == 0 )
        {
            if ( block != 0 )
            {
                if ( _iguana_chainlink(coin,block) <= 0 )
                {
                    iguana_blockunmark(coin,block,bp,0,0);
                    bp->issued[0] = 0;
                    hash2 = bp->hashes[0];
                    //char str[65]; printf("RT[0] [%d:%d] %s %p\n",bp->hdrsi,0,bits256_str(str,hash2),block);
                    addr = coin->peers.ranked[rand() % 8];
                    if ( addr != 0 && addr->usock >= 0 && addr->dead == 0 )
                        iguana_sendblockreqPT(coin,addr,bp,0,hash2,0);
                }
            }
        }
        //char str[65]; printf("check longest.%d RTheight.%d hwm.%d %s %p\n",coin->longestchain,coin->RTheight,coin->blocks.hwmchain.height,bits256_str(str,bp->hashes[0]),block);
        if ( bits256_cmp(coin->RThash1,bp->hashes[1]) != 0 )
            coin->RThash1 = bp->hashes[1];
        bp->lastRT = (uint32_t)time(NULL);
        if ( coin->RTheight < coin->longestchain && coin->peers.numranked > 0 && time(NULL) > coin->RThdrstime+10 )
        {
            iguana_RThdrs(coin,bp,coin->peers.numranked);
            coin->RThdrstime = bp->lastRT;
            for (i=0; i<coin->peers.numranked; i++)
            {
                if ( (addr= coin->peers.ranked[i]) != 0 && addr->usock >= 0 && addr->dead == 0 )
                {
                    //printf("%d ",addr->numRThashes);
                }
            }
            //printf("RTheaders %s\n",coin->symbol);
        }
        bp->lastRT = (uint32_t)time(NULL);
        iguana_RTramchainalloc("RTbundle",coin,bp);
        bp->isRT = 1;
        while ( (rdata= coin->RTramchain.H.data) != 0 && coin->RTheight <= coin->blocks.hwmchain.height )
        {
            if ( coin->RTdatabad != 0 )
                break;
            dest = &coin->RTramchain;
            B = (void *)(long)((long)rdata + rdata->Boffset);
            bundlei = (coin->RTheight % coin->chain->bundlesize);
            if ( (block= iguana_bundleblock(coin,&hash2,bp,bundlei)) != 0 )
                iguana_bundlehashadd(coin,bp,bundlei,block);
            //printf("RT.%d vs hwm.%d starti.%d bp->n %d block.%p/%p ramchain.%p\n",coin->RTheight,coin->blocks.hwmchain.height,coin->RTstarti,bp->n,block,bp->blocks[bundlei],dest->H.data);
            if ( coin->RTdatabad == 0 && block != 0 && bits256_nonz(block->RO.prev_block) != 0 )
            {
                iguana_blocksetcounters(coin,block,dest);
                startmillis0 = OS_milliseconds();
                if ( coin->RTdatabad == 0 && iguana_ramchainfile(coin,dest,&blockR,bp,bundlei,block) == 0 )
                {
                    for (i=0; i<bp->n; i++)
                        if ( GETBIT(bp->haveblock,i) == 0 )
                            bp->issued[i] = 0;
                    if (  (n= iguana_bundleissuemissing(coin,bp,3,1.)) > 0 )
                        printf("RT issued %d priority requests [%d] to unstick stuckiters.%d\n",n,bp->hdrsi,coin->stuckiters);
                    for (i=bundlei; i<bp->n; i++)
                    {
                        block = iguana_bundleblock(coin,&hash2,bp,i);
                        if ( bits256_nonz(hash2) != 0 && (block == 0 || block->txvalid == 0) )
                        {
                            uint8_t serialized[512]; int32_t len; struct iguana_peer *addr;
                            //char str[65]; printf("RT error [%d:%d] %s %p\n",bp->hdrsi,i,bits256_str(str,hash2),block);
                            addr = coin->peers.ranked[rand() % 8];
                            if ( addr != 0 && addr->usock >= 0 && addr->dead == 0 && (len= iguana_getdata(coin,serialized,MSG_BLOCK,&hash2,1)) > 0 )
                                iguana_send(coin,addr,serialized,len);
                            coin->RTgenesis = 0;
                        }
                        if ( bits256_nonz(hash2) != 0 )
                            iguana_blockQ("RTerr",coin,bp,i,hash2,1);
                        break;
                    }
                    return(-1);
                } else iguana_ramchain_free(coin,&blockR,1);
                B[bundlei] = block->RO;
                totalmillis0 += (OS_milliseconds() - startmillis0);
                num0++;
                flag++;
                coin->blocks.RO[bp->bundleheight+bundlei] = block->RO;
                coin->RTheight++;
                printf(">>>> RT.%d hwm.%d L.%d T.%d U.%d S.%d P.%d X.%d -> size.%ld\n",coin->RTheight,coin->blocks.hwmchain.height,coin->longestchain,dest->H.txidind,dest->H.unspentind,dest->H.spendind,dest->pkind,dest->externalind,(long)dest->H.data->allocsize);
                if ( coin->RTramchain.H.data != 0 )
                    coin->RTramchain.H.data->numblocks = bundlei + 1;
                else break;
            } else break;
        }
    }
    n = 0;
    if ( coin->RTdatabad == 0 && dest != 0 && flag != 0 && coin->RTheight >= coin->longestchain )
    {
        //printf("ramchainiterate.[%d] ave %.2f micros, total %.2f seconds starti.%d num.%d\n",num0,(totalmillis0*1000.)/num0,totalmillis0/1000.,coin->RTstarti,coin->RTheight%bp->n);
        if ( (n= iguana_walkchain(coin,1)) == coin->RTheight-1 )
        {
            //printf("RTgenesis verified\n");
            coin->RTgenesis = (uint32_t)time(NULL);
            iguana_RTspendvectors(coin,bp);
        } else coin->RTdatabad = 1;
    }
    if ( dest != 0 && flag != 0 )
        printf("<<<< flag.%d RT.%d:%d hwm.%d L.%d T.%d U.%d S.%d P.%d X.%d -> size.%ld\n",flag,coin->RTheight,n,coin->blocks.hwmchain.height,coin->longestchain,dest->H.txidind,dest->H.unspentind,dest->H.spendind,dest->pkind,dest->externalind,dest->H.data!=0?(long)dest->H.data->allocsize:-1);
    if ( coin->RTdatabad != 0 )
    {
        iguana_RTramchainfree(coin,bp);
        //iguana_RTramchainalloc("RTbundle",coin,bp);
    }
    return(flag);
}

int32_t iguana_bundlevalidate(struct iguana_info *coin,struct iguana_bundle *bp,int32_t forceflag)
{
    static int32_t totalerrs,totalvalidated;
    FILE *fp; char fname[1024]; uint8_t *blockspace; uint32_t now = (uint32_t)time(NULL);
    int32_t i,max,len,errs = 0; struct sha256_vstate vstate; bits256 validatehash; int64_t total = 0;
    if ( bp->ramchain.from_ro != 0 || bp == coin->current )
    {
        bp->validated = (uint32_t)time(NULL);
        return(bp->n);
    }
    if ( bp->validated <= 1 || forceflag != 0 )
    {
        //printf("validate.[%d]\n",bp->hdrsi);
        vupdate_sha256(validatehash.bytes,&vstate,0,0);
        sprintf(fname,"%s/%s/validated/%d",GLOBAL_DBDIR,coin->symbol,bp->bundleheight);
        //printf("validatefname.(%s)\n",fname);
        if ( (fp= fopen(fname,"rb")) != 0 )
        {
            if ( forceflag == 0 )
            {
                if ( fread(&bp->validated,1,sizeof(bp->validated),fp) != sizeof(bp->validated) ||fread(&total,1,sizeof(total),fp) != sizeof(total) || fread(&validatehash,1,sizeof(validatehash),fp) != sizeof(validatehash) )
                {
                    printf("error reading.(%s)\n",fname);
                    total = bp->validated = 0;
                } //else printf("(%s) total.%d validated.%u\n",fname,(int32_t)total,bp->validated);
            } else OS_removefile(fname,1);
            fclose(fp);
        }
        if ( forceflag != 0 || bp->validated <= 1 )
        {
            max = sizeof(coin->blockspace);
            blockspace = calloc(1,max);
            for (i=0; i<bp->n; i++)
            {
                if ( (len= iguana_peerblockrequest(coin,blockspace,max,0,bp->hashes[i],1)) < 0 )
                {
                    errs++;
                    iguana_blockunmark(coin,bp->blocks[i],bp,i,1);
                    totalerrs++;
                }
                else
                {
                    vupdate_sha256(validatehash.bytes,&vstate,bp->hashes[i].bytes,sizeof(bp->hashes[i]));
                    total += len, totalvalidated++;
                }
            }
            free(blockspace);
            bp->validated = (uint32_t)time(NULL);
            printf("VALIDATED.[%d] ht.%d duration.%d errs.%d total.%lld %u | total errs.%d validated.%d %llx\n",bp->hdrsi,bp->bundleheight,bp->validated - now,errs,(long long)total,bp->validated,totalerrs,totalvalidated,(long long)validatehash.txid);
        }
        if ( errs == 0 && fp == 0 )
        {
            if ( (fp= fopen(fname,"wb")) != 0 )
            {
                if ( fwrite(&bp->validated,1,sizeof(bp->validated),fp) != sizeof(bp->validated) || fwrite(&total,1,sizeof(total),fp) != sizeof(total) || fwrite(&validatehash,1,sizeof(validatehash),fp) != sizeof(validatehash) )
                    printf("error saving.(%s) total.%lld\n",fname,(long long)total);
                fclose(fp);
            }
        }
        bp->validatehash = validatehash;
    } // else printf("skip validate.[%d] validated.%u force.%d\n",bp->hdrsi,bp->validated,forceflag);
    if ( errs != 0 )
    {
        printf("remove.[%d]\n",bp->hdrsi);
        iguana_bundleremove(coin,bp->hdrsi,1);
    }
    return(bp->n - errs);
}

#include "../includes/iguana_apidefs.h"
#include "../includes/iguana_apideclares.h"

STRING_ARG(iguana,initfastfind,activecoin)
{
    if ( (coin= iguana_coinfind(activecoin)) != 0 )
    {
        iguana_fastfindcreate(coin);
        return(clonestr("{\"result\":\"fast find initialized\"}"));
    } else return(clonestr("{\"error\":\"no coin to initialize\"}"));
}

TWOSTRINGS_AND_INT(iguana,balance,activecoin,address,height)
{
    int32_t minconf=1,maxconf=SATOSHIDEN; int64_t total; uint8_t rmd160[20],pubkey33[33],addrtype;
    struct iguana_pkhash *P; cJSON *array,*retjson = cJSON_CreateObject();
    if ( activecoin != 0 && activecoin[0] != 0 )
        coin = iguana_coinfind(activecoin);
    if ( coin != 0 )
    {
        jaddstr(retjson,"address",address);
        if ( bitcoin_validaddress(coin,address) < 0 )
        {
            jaddstr(retjson,"error","illegal address");
            return(jprint(retjson,1));
        }
        if ( bitcoin_addr2rmd160(&addrtype,rmd160,address) < 0 )
        {
            jaddstr(retjson,"error","cant convert address");
            return(jprint(retjson,1));
        }
        memset(pubkey33,0,sizeof(pubkey33));
        P = calloc(coin->bundlescount,sizeof(*P));
        array = cJSON_CreateArray();
        printf("Start %s balance.(%s) height.%d\n",coin->symbol,address,height);
        if ( height == 0 )
            height = (1 << 30);
        iguana_pkhasharray(coin,array,minconf,maxconf,&total,P,coin->bundlescount,rmd160,address,pubkey33,height);
        free(P);
        jadd(retjson,"unspents",array);
        jaddnum(retjson,"balance",dstr(total));
        if ( height != 0 )
            jaddnum(retjson,"height",height);
    }
    return(jprint(retjson,1));
}
#include "../includes/iguana_apiundefs.h"
