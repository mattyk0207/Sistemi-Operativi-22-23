#include "include/ash.h"

static semd_t semd_table[MAXPROC];

static semd_t *semdFree_h;

DEFINE_HASHTABLE(semd_h, 5);



int insertBlocked(int *semAdd, pcb_t *p)
{
    int key = hash_min((int)semAdd, HASH_BITS(semd_h));
    int empty = hlist_empty(&semd_h[key]);
    if (!empty)
    {
        semd_t *tmp = container_of(semd_h[key].first, semd_t, s_link);
        p->p_semAdd = semAdd;
        insertProcQ(&(tmp->s_procq), p);
        return false;
    }
    else if (&semdFree_h->s_freelink != NULL)
    {
        semd_t *tmp = semdFree_h;
        semdFree_h = container_of(semdFree_h->s_freelink.next, semd_t, s_freelink);
        //list_del(tmp);

        tmp->s_freelink.next = NULL;
        tmp->s_key = semAdd;
        mkEmptyProcQ(&(tmp->s_procq));
        p->p_semAdd = semAdd;
        insertProcQ(&(tmp->s_procq), p);

        hlist_add_head(&tmp->s_link, &semd_h[key]);
        return false;
    }
    else
        return true;
}

pcb_t *removeBlocked(int *semAdd)
{
    semd_t *t;
    hash_for_each_possible(semd_h, t, s_link, (int)semAdd)
    {
        if (t != NULL)
        {
            pcb_t *p = removeProcQ(&(t->s_procq));
            //se il SEMD ora è vuoto lo rimuove
            if (emptyProcQ(&(t->s_procq)))
            {
                hash_del(&(t->s_link));
                t->s_freelink.next = &(semdFree_h->s_freelink);
                semdFree_h = t;
            }
            return p;
        }
    }
    return NULL;
}

pcb_t *outBlocked(pcb_t *p)
{
    semd_t *t;
    hash_for_each_possible(semd_h, t, s_link, (int)p->p_semAdd)
    {
        pcb_t *out  = outProcQ(&t->s_procq, p);
        //se il SEMD ora è vuoto lo rimuove
        if (emptyProcQ(&(t->s_procq)))
            {
                hash_del(&(t->s_link));
                t->s_freelink.next = &(semdFree_h->s_freelink);
                semdFree_h = t;
            }
        return out;
    }
    return NULL;
}

pcb_t *headBlocked(int *semAdd)
{
    semd_t *t;
    hash_for_each_possible(semd_h, t, s_link, (int)semAdd) {
        return headProcQ(&t->s_procq);
    }
    return NULL;
}

void initASH()
{
    semdFree_h = &semd_table[0];
    INIT_LIST_HEAD(&semdFree_h->s_freelink);
    semd_t *t = semdFree_h;
    struct list_head *p = t->s_freelink.prev;
    for (int i = 1; i < MAXPROC; i++)
    {
        t->s_freelink.next = &semd_table[i].s_freelink;
        t = container_of(t->s_freelink.next, semd_t, s_freelink);
        t->s_freelink.prev = p;
        t->s_freelink.next = NULL;
        p = p->next;
    }
    /*
    semdFree_h = &semd_table[0];
    semdFree_h->s_freelink.next = NULL;
    semdFree_h->s_freelink.prev = NULL;
    semd_t *tmp = semdFree_h;
    for (int i = 1; i < MAXPROC; i++)
    {
        tmp->s_freelink.next = &(semd_table[i].s_freelink);
        tmp = container_of(tmp->s_freelink.next, semd_t, s_freelink);
        tmp->s_freelink.prev = NULL;
        tmp->s_freelink.next = NULL;
    }*/
}