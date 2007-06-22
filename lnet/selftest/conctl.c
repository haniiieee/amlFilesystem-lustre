/* -*- mode: c; c-basic-offset: 8; indent-tabs-mode: nil; -*-
 * vim:expandtab:shiftwidth=8:tabstop=8:
 *
 * Author: Liang Zhen <liangzhen@clusterfs.com>
 *
 * This file is part of Lustre, http://www.lustre.org
 *
 * IOC handle in kernel 
 */
#ifdef __KERNEL__

#include <libcfs/libcfs.h>
#include <lnet/lib-lnet.h>
#include <lnet/lnetst.h>
#include "console.h"

int
lst_session_new_ioctl(lstio_session_new_args_t *args) 
{
        char      *name;
        int        rc;

        if (args->lstio_ses_idp   == NULL || /* address for output sid */
            args->lstio_ses_key   == 0 || /* no key is specified */
            args->lstio_ses_namep == NULL || /* session name */
            args->lstio_ses_nmlen <= 0 ||
            args->lstio_ses_nmlen > LST_NAME_SIZE)
                return -EINVAL;
       
        LIBCFS_ALLOC(name, args->lstio_ses_nmlen + 1);
        if (name == NULL)
                return -ENOMEM;
        
        if (copy_from_user(name,
                           args->lstio_ses_namep,
                           args->lstio_ses_nmlen)) {
                LIBCFS_FREE(name, args->lstio_ses_nmlen + 1);
                return -EFAULT;
        }
        
        name[args->lstio_ses_nmlen] = 0;
        
        rc = lstcon_session_new(name,
                             args->lstio_ses_key,
                             args->lstio_ses_timeout,
                             args->lstio_ses_force,
                             args->lstio_ses_idp);
        
        LIBCFS_FREE(name, args->lstio_ses_nmlen + 1);

        return rc;
}

int
lst_session_end_ioctl(lstio_session_end_args_t *args)
{
        if (args->lstio_ses_key != console_session.ses_key)
                return -EACCES;

        return lstcon_session_end();
}

int
lst_session_info_ioctl(lstio_session_info_args_t *args)
{
        /* no checking of key */

        if (args->lstio_ses_idp   == NULL || /* address for ouput sid */
            args->lstio_ses_keyp  == NULL || /* address for ouput key */
            args->lstio_ses_ndinfo == NULL || /* address for output ndinfo */
            args->lstio_ses_namep == NULL || /* address for ouput name */
            args->lstio_ses_nmlen <= 0 ||
            args->lstio_ses_nmlen > LST_NAME_SIZE)
                return -EINVAL;

        return lstcon_session_info(args->lstio_ses_idp,
                                   args->lstio_ses_keyp,
                                   args->lstio_ses_ndinfo,
                                   args->lstio_ses_namep,
                                   args->lstio_ses_nmlen);
}

int
lst_debug_ioctl(lstio_debug_args_t *args)
{
        char   *name   = NULL;
        int     client = 1;
        int     rc;

        if (args->lstio_dbg_key != console_session.ses_key)
                return -EACCES;

        if (args->lstio_dbg_resultp == NULL)
                return -EINVAL;

        if (args->lstio_dbg_namep != NULL && /* name of batch/group */
            (args->lstio_dbg_nmlen <= 0 ||
             args->lstio_dbg_nmlen > LST_NAME_SIZE))
                return -EINVAL;

        if (args->lstio_dbg_namep != NULL) {
                LIBCFS_ALLOC(name, args->lstio_dbg_nmlen + 1);
                if (name == NULL)
                        return -ENOMEM;

                if (copy_from_user(name, args->lstio_dbg_namep,
                                   args->lstio_dbg_nmlen)) {
                        LIBCFS_FREE(name, args->lstio_dbg_nmlen + 1);

                        return -EFAULT;
                }

                name[args->lstio_dbg_nmlen] = 0;
        }

        rc = -EINVAL;

        switch (args->lstio_dbg_type) {
        case LST_OPC_SESSION:
                rc = lstcon_session_debug(args->lstio_dbg_timeout,
                                          args->lstio_dbg_resultp);
                break;

        case LST_OPC_BATCHSRV:
                client = 0;
        case LST_OPC_BATCHCLI:
                if (name == NULL)
                        goto out;

                rc = lstcon_batch_debug(args->lstio_dbg_timeout,
                                        name, client, args->lstio_dbg_resultp);
                break;

        case LST_OPC_GROUP:
                if (name == NULL)
                        goto out;

                rc = lstcon_group_debug(args->lstio_dbg_timeout,
                                        name, args->lstio_dbg_resultp);
                break;

        case LST_OPC_NODES:
                if (args->lstio_dbg_count <= 0 ||
                    args->lstio_dbg_idsp == NULL)
                        goto out;

                rc = lstcon_nodes_debug(args->lstio_dbg_timeout,
                                        args->lstio_dbg_count,
                                        args->lstio_dbg_idsp,
                                        args->lstio_dbg_resultp);
                break;

        default:
                break;
        }

out:
        if (name != NULL)
                LIBCFS_FREE(name, args->lstio_dbg_nmlen + 1);

        return rc;
}

int
lst_group_add_ioctl(lstio_group_add_args_t *args)
{
        char           *name;
        int             rc;

        if (args->lstio_grp_key != console_session.ses_key)
                return -EACCES;

        if (args->lstio_grp_namep == NULL||
            args->lstio_grp_nmlen <= 0 || 
            args->lstio_grp_nmlen > LST_NAME_SIZE)
                return -EINVAL;

        LIBCFS_ALLOC(name, args->lstio_grp_nmlen + 1);
        if (name == NULL)
                return -ENOMEM;

        if (copy_from_user(name,
                           args->lstio_grp_namep,
                           args->lstio_grp_nmlen)) {
                LIBCFS_FREE(name, args->lstio_grp_nmlen);
                return -EFAULT;
        }

        name[args->lstio_grp_nmlen] = 0;

        rc = lstcon_group_add(name);

        LIBCFS_FREE(name, args->lstio_grp_nmlen + 1);

        return rc;
}

int
lst_group_del_ioctl(lstio_group_del_args_t *args)
{
        int     rc;
        char   *name;

        if (args->lstio_grp_key != console_session.ses_key)
                return -EACCES;
        
        if (args->lstio_grp_namep == NULL ||
            args->lstio_grp_nmlen <= 0 || 
            args->lstio_grp_nmlen > LST_NAME_SIZE)
                return -EINVAL;

        LIBCFS_ALLOC(name, args->lstio_grp_nmlen + 1);
        if (name == NULL)
                return -ENOMEM;

        if (copy_from_user(name,
                           args->lstio_grp_namep,
                           args->lstio_grp_nmlen)) {
                LIBCFS_FREE(name, args->lstio_grp_nmlen + 1);
                return -EFAULT;
        }

        name[args->lstio_grp_nmlen] = 0;

        rc = lstcon_group_del(name);

        LIBCFS_FREE(name, args->lstio_grp_nmlen + 1);

        return rc;
}

int
lst_group_update_ioctl(lstio_group_update_args_t *args)
{
        int     rc;
        char   *name;

        if (args->lstio_grp_key != console_session.ses_key)
                return -EACCES;

        if (args->lstio_grp_resultp == NULL ||
            args->lstio_grp_namep == NULL ||
            args->lstio_grp_nmlen <= 0 || 
            args->lstio_grp_nmlen > LST_NAME_SIZE)
                return -EINVAL;

        LIBCFS_ALLOC(name, args->lstio_grp_nmlen + 1);
        if (name == NULL)
                return -ENOMEM;

        if (copy_from_user(name,
                           args->lstio_grp_namep,
                           args->lstio_grp_nmlen)) {
                LIBCFS_FREE(name, args->lstio_grp_nmlen + 1);
                return -EFAULT;
        }

        name[args->lstio_grp_nmlen] = 0;

        switch (args->lstio_grp_opc) {
        case LST_GROUP_CLEAN:
                rc = lstcon_group_clean(name, args->lstio_grp_args);
                break;

        case LST_GROUP_REFRESH:
                rc = lstcon_group_refresh(name, args->lstio_grp_resultp);
                break;

        case LST_GROUP_RMND:
                if (args->lstio_grp_count  <= 0 ||
                    args->lstio_grp_idsp == NULL) {
                        rc = -EINVAL;
                        break;
                }
                rc = lstcon_nodes_remove(name, args->lstio_grp_count,
                                         args->lstio_grp_idsp,
                                         args->lstio_grp_resultp);
                break;

        default:
                rc = -EINVAL;
                break;
        }

        LIBCFS_FREE(name, args->lstio_grp_nmlen + 1);
        
        return rc;
}

int
lst_nodes_add_ioctl(lstio_group_nodes_args_t *args)
{
        int     rc;
        char   *name;

        if (args->lstio_grp_key != console_session.ses_key)
                return -EACCES;

        if (args->lstio_grp_idsp == NULL || /* array of ids */
            args->lstio_grp_count <= 0 ||
            args->lstio_grp_resultp == NULL ||
            args->lstio_grp_namep == NULL ||
            args->lstio_grp_nmlen <= 0 || 
            args->lstio_grp_nmlen > LST_NAME_SIZE)
                return -EINVAL;

        LIBCFS_ALLOC(name, args->lstio_grp_nmlen + 1);
        if (name == NULL)
                return -ENOMEM;

        if (copy_from_user(name, args->lstio_grp_namep,
                           args->lstio_grp_nmlen)) {
                LIBCFS_FREE(name, args->lstio_grp_nmlen + 1);

                return -EFAULT;
        }

        name[args->lstio_grp_nmlen] = 0;

        rc = lstcon_nodes_add(name, args->lstio_grp_count,
                              args->lstio_grp_idsp,
                              args->lstio_grp_resultp);

        LIBCFS_FREE(name, args->lstio_grp_nmlen + 1);

        return rc;
}

int
lst_group_list_ioctl(lstio_group_list_args_t *args)
{
        if (args->lstio_grp_key != console_session.ses_key) 
                return -EACCES;

        if (args->lstio_grp_idx   < 0 ||
            args->lstio_grp_namep == NULL ||
            args->lstio_grp_nmlen <= 0 ||
            args->lstio_grp_nmlen > LST_NAME_SIZE)
                return -EINVAL;

        return lstcon_group_list(args->lstio_grp_idx,
                              args->lstio_grp_nmlen,
                              args->lstio_grp_namep);
}

int
lst_group_info_ioctl(lstio_group_info_args_t *args)
{
        char           *name;
        int             ndent;
        int             index;
        int             rc;

        if (args->lstio_grp_key != console_session.ses_key)
                return -EACCES;

        if (args->lstio_grp_namep == NULL ||
            args->lstio_grp_nmlen <= 0 ||
            args->lstio_grp_nmlen > LST_NAME_SIZE)
                return -EINVAL;

        if (args->lstio_grp_entp  == NULL && /* output: group entry */
            args->lstio_grp_dentsp == NULL)  /* output: node entry */
                return -EINVAL;

        if (args->lstio_grp_dentsp != NULL) { /* have node entry */
                if (args->lstio_grp_idxp == NULL || /* node index */
                    args->lstio_grp_ndentp == NULL) /* # of node entry */
                        return -EINVAL;
                                
                if (copy_from_user(&ndent,
                                   args->lstio_grp_ndentp, sizeof(ndent)) ||
                    copy_from_user(&index, args->lstio_grp_idxp, sizeof(index)))
                        return -EFAULT;

                if (ndent <= 0 || index < 0)
                        return -EINVAL;
        }

        LIBCFS_ALLOC(name, args->lstio_grp_nmlen + 1);
        if (name == NULL)
                return -ENOMEM;

        if (copy_from_user(name,
                           args->lstio_grp_namep,
                           args->lstio_grp_nmlen)) {
                LIBCFS_FREE(name, args->lstio_grp_nmlen + 1);
                return -EFAULT;
        }

        name[args->lstio_grp_nmlen] = 0;

        rc = lstcon_group_info(name, args->lstio_grp_entp,
                               &index, &ndent, args->lstio_grp_dentsp);

        LIBCFS_FREE(name, args->lstio_grp_nmlen + 1);

        if (rc != 0) 
                return rc;

        if (args->lstio_grp_dentsp != NULL && 
            (copy_to_user(args->lstio_grp_idxp, &index, sizeof(index)) ||
             copy_to_user(args->lstio_grp_ndentp, &ndent, sizeof(ndent))))
                rc = -EFAULT;

        return 0;
}

int
lst_batch_add_ioctl(lstio_batch_add_args_t *args)
{
        int             rc;
        char           *name;

        if (args->lstio_bat_key != console_session.ses_key)
                return -EACCES;

        if (args->lstio_bat_namep == NULL ||
            args->lstio_bat_nmlen <= 0 ||
            args->lstio_bat_nmlen > LST_NAME_SIZE)
                return -EINVAL;

        LIBCFS_ALLOC(name, args->lstio_bat_nmlen + 1);
        if (name == NULL)
                return -ENOMEM;

        if (copy_from_user(name,
                           args->lstio_bat_namep,
                           args->lstio_bat_nmlen)) {
                LIBCFS_FREE(name, args->lstio_bat_nmlen + 1);
                return -EFAULT;
        }

        name[args->lstio_bat_nmlen] = 0;

        rc = lstcon_batch_add(name);

        LIBCFS_FREE(name, args->lstio_bat_nmlen + 1);

        return rc;
}

int
lst_batch_run_ioctl(lstio_batch_run_args_t *args)
{
        int             rc;
        char           *name;

        if (args->lstio_bat_key != console_session.ses_key)
                return -EACCES;

        if (args->lstio_bat_namep == NULL ||
            args->lstio_bat_nmlen <= 0 ||
            args->lstio_bat_nmlen > LST_NAME_SIZE)
                return -EINVAL;

        LIBCFS_ALLOC(name, args->lstio_bat_nmlen + 1);
        if (name == NULL)
                return -ENOMEM;

        if (copy_from_user(name,
                           args->lstio_bat_namep,
                           args->lstio_bat_nmlen)) {
                LIBCFS_FREE(name, args->lstio_bat_nmlen + 1);
                return -EFAULT;
        }

        name[args->lstio_bat_nmlen] = 0;

        rc = lstcon_batch_run(name, args->lstio_bat_timeout,
                              args->lstio_bat_resultp);

        LIBCFS_FREE(name, args->lstio_bat_nmlen + 1);

        return rc;
}

int
lst_batch_stop_ioctl(lstio_batch_stop_args_t *args)
{
        int             rc;
        char           *name;

        if (args->lstio_bat_key != console_session.ses_key)
                return -EACCES;

        if (args->lstio_bat_resultp == NULL ||
            args->lstio_bat_namep == NULL ||
            args->lstio_bat_nmlen <= 0 ||
            args->lstio_bat_nmlen > LST_NAME_SIZE)
                return -EINVAL;

        LIBCFS_ALLOC(name, args->lstio_bat_nmlen + 1);
        if (name == NULL)
                return -ENOMEM;

        if (copy_from_user(name,
                           args->lstio_bat_namep,
                           args->lstio_bat_nmlen)) {
                LIBCFS_FREE(name, args->lstio_bat_nmlen + 1);
                return -EFAULT;
        }

        name[args->lstio_bat_nmlen] = 0;

        rc = lstcon_batch_stop(name, args->lstio_bat_force,
                               args->lstio_bat_resultp);

        LIBCFS_FREE(name, args->lstio_bat_nmlen + 1);

        return rc;
}

int
lst_batch_query_ioctl(lstio_batch_query_args_t *args)
{
        char   *name;
        int     rc;

        if (args->lstio_bat_key != console_session.ses_key)
                return -EACCES;

        if (args->lstio_bat_resultp == NULL ||
            args->lstio_bat_namep == NULL ||
            args->lstio_bat_nmlen <= 0 ||
            args->lstio_bat_nmlen > LST_NAME_SIZE)
                return -EINVAL;

        if (args->lstio_bat_testidx < 0)
                return -EINVAL;

        LIBCFS_ALLOC(name, args->lstio_bat_nmlen + 1);
        if (name == NULL)
                return -ENOMEM;

        if (copy_from_user(name,
                           args->lstio_bat_namep,
                           args->lstio_bat_nmlen)) {
                LIBCFS_FREE(name, args->lstio_bat_nmlen + 1);
                return -EFAULT;
        }

        name[args->lstio_bat_nmlen] = 0;

        rc = lstcon_test_batch_query(name,
                                     args->lstio_bat_testidx,
                                     args->lstio_bat_client,
                                     args->lstio_bat_timeout,
                                     args->lstio_bat_resultp);

        LIBCFS_FREE(name, args->lstio_bat_nmlen + 1);

        return rc;
}

int
lst_batch_list_ioctl(lstio_batch_list_args_t *args)
{
        if (args->lstio_bat_key != console_session.ses_key)
                return -EACCES;

        if (args->lstio_bat_idx   < 0 ||
            args->lstio_bat_namep == NULL ||
            args->lstio_bat_nmlen <= 0 ||
            args->lstio_bat_nmlen > LST_NAME_SIZE)
                return -EINVAL;

        return lstcon_batch_list(args->lstio_bat_idx,
                              args->lstio_bat_nmlen,
                              args->lstio_bat_namep);
}

int
lst_batch_info_ioctl(lstio_batch_info_args_t *args)
{
        char           *name;
        int             rc;
        int             index;
        int             ndent;

        if (args->lstio_bat_key != console_session.ses_key)
                return -EACCES;

        if (args->lstio_bat_namep == NULL || /* batch name */
            args->lstio_bat_nmlen <= 0 ||
            args->lstio_bat_nmlen > LST_NAME_SIZE)
                return -EINVAL;

        if (args->lstio_bat_entp == NULL && /* output: batch entry */
            args->lstio_bat_dentsp == NULL) /* output: node entry */
                return -EINVAL;

        if (args->lstio_bat_dentsp != NULL) { /* have node entry */
                if (args->lstio_bat_idxp == NULL || /* node index */
                    args->lstio_bat_ndentp == NULL) /* # of node entry */
                        return -EINVAL;
                                
                if (copy_from_user(&index, args->lstio_bat_idxp, sizeof(index)) ||
                    copy_from_user(&ndent, args->lstio_bat_ndentp, sizeof(ndent)))
                        return -EFAULT;

                if (ndent <= 0 || index < 0)
                        return -EINVAL;
        }

        LIBCFS_ALLOC(name, args->lstio_bat_nmlen + 1);
        if (name == NULL)
                return -ENOMEM;

        if (copy_from_user(name,
                           args->lstio_bat_namep, args->lstio_bat_nmlen)) {
                LIBCFS_FREE(name, args->lstio_bat_nmlen + 1);
                return -EFAULT;
        }

        name[args->lstio_bat_nmlen] = 0;

        rc = lstcon_batch_info(name,
                            args->lstio_bat_entp, args->lstio_bat_server,
                            args->lstio_bat_testidx, &index, &ndent,
                            args->lstio_bat_dentsp);

        LIBCFS_FREE(name, args->lstio_bat_nmlen + 1);

        if (rc != 0)
                return rc;

        if (args->lstio_bat_dentsp != NULL && 
            (copy_to_user(args->lstio_bat_idxp, &index, sizeof(index)) ||
             copy_to_user(args->lstio_bat_ndentp, &ndent, sizeof(ndent))))
                rc = -EFAULT;

        return rc;
}

int
lst_stat_query_ioctl(lstio_stat_args_t *args)
{
        int             rc;
        char           *name;

        /* TODO: not finished */
        if (args->lstio_sta_key != console_session.ses_key)
                return -EACCES;

        if (args->lstio_sta_resultp == NULL ||
            (args->lstio_sta_namep  == NULL &&
             args->lstio_sta_idsp   == NULL) ||
            args->lstio_sta_nmlen <= 0 ||
            args->lstio_sta_nmlen > LST_NAME_SIZE)
                return -EINVAL;

        if (args->lstio_sta_idsp != NULL &&
            args->lstio_sta_count <= 0)
                return -EINVAL;

        LIBCFS_ALLOC(name, args->lstio_sta_nmlen + 1);
        if (name == NULL)
                return -ENOMEM;

        if (copy_from_user(name, args->lstio_sta_namep,
                           args->lstio_sta_nmlen)) {
                LIBCFS_FREE(name, args->lstio_sta_nmlen + 1);
                return -EFAULT;
        }

        if (args->lstio_sta_idsp == NULL) {
                rc = lstcon_group_stat(name, args->lstio_sta_timeout,
                                       args->lstio_sta_resultp);
        } else {
                rc = lstcon_nodes_stat(args->lstio_sta_count,
                                       args->lstio_sta_idsp,
                                       args->lstio_sta_timeout,
                                       args->lstio_sta_resultp);
        }

        LIBCFS_FREE(name, args->lstio_sta_nmlen + 1);

        return rc;
}

int lst_test_add_ioctl(lstio_test_args_t *args)
{
        char           *name;
        char           *srcgrp = NULL;
        char           *dstgrp = NULL;
        void           *param = NULL;
        int             rc = -ENOMEM;

        if (args->lstio_tes_resultp == NULL ||
            args->lstio_tes_bat_name == NULL || /* no specified batch */
            args->lstio_tes_bat_nmlen <= 0 ||
            args->lstio_tes_bat_nmlen > LST_NAME_SIZE ||
            args->lstio_tes_sgrp_name == NULL || /* no source group */
            args->lstio_tes_sgrp_nmlen <= 0 ||
            args->lstio_tes_sgrp_nmlen > LST_NAME_SIZE ||
            args->lstio_tes_dgrp_name == NULL || /* no target group */
            args->lstio_tes_dgrp_nmlen <= 0 ||
            args->lstio_tes_dgrp_nmlen > LST_NAME_SIZE)
                return -EINVAL;

        /* have parameter, check if parameter length is valid */
        if (args->lstio_tes_param != NULL &&
            (args->lstio_tes_param_len <= 0 ||
             args->lstio_tes_param_len > CFS_PAGE_SIZE - sizeof(lstcon_test_t)))
                return -EINVAL;

        LIBCFS_ALLOC(name, args->lstio_tes_bat_nmlen + 1);
        if (name == NULL)
                return rc;

        LIBCFS_ALLOC(srcgrp, args->lstio_tes_sgrp_nmlen + 1);
        if (srcgrp == NULL) 
                goto out;

        LIBCFS_ALLOC(dstgrp, args->lstio_tes_dgrp_nmlen + 1);
        if (srcgrp == NULL) 
                goto out;

        if (args->lstio_tes_param != NULL) {
                LIBCFS_ALLOC(param, args->lstio_tes_param_len);
                if (param == NULL) 
                        goto out;
        }

        rc = -EFAULT;
        if (copy_from_user(name,
                           args->lstio_tes_bat_name,
                           args->lstio_tes_bat_nmlen) ||
            copy_from_user(srcgrp,
                           args->lstio_tes_sgrp_name,
                           args->lstio_tes_sgrp_nmlen) ||
            copy_from_user(dstgrp,
                           args->lstio_tes_dgrp_name,
                           args->lstio_tes_dgrp_nmlen) ||
            copy_from_user(param, args->lstio_tes_param,
                           args->lstio_tes_param_len))
                goto out;

        rc = lstcon_test_add(name,
                            args->lstio_tes_type,
                            args->lstio_tes_loop,
                            args->lstio_tes_concur,
                            args->lstio_tes_dist, args->lstio_tes_span,
                            srcgrp, dstgrp, param, args->lstio_tes_param_len,
                            args->lstio_tes_resultp);
out:
        if (name != NULL)
                LIBCFS_FREE(name, args->lstio_tes_bat_nmlen + 1);

        if (srcgrp != NULL)
                LIBCFS_FREE(srcgrp, args->lstio_tes_sgrp_nmlen + 1);

        if (dstgrp != NULL)
                LIBCFS_FREE(dstgrp, args->lstio_tes_dgrp_nmlen + 1);

        if (param != NULL)
                LIBCFS_FREE(param, args->lstio_tes_param_len);

        return rc;
}

int
lstcon_ioctl_entry(unsigned int cmd, struct libcfs_ioctl_data *data)
{
        char   *buf;
        int     opc = data->ioc_u32[0];
        int     rc;

        if (cmd != IOC_LIBCFS_LNETST)
                return -EINVAL;

        if (data->ioc_plen1 > CFS_PAGE_SIZE)
                return -EINVAL;

        LIBCFS_ALLOC(buf, data->ioc_plen1);
        if (buf == NULL)
                return -ENOMEM;

        /* copy in parameter */
        if (copy_from_user(buf, data->ioc_pbuf1, data->ioc_plen1)) {
                LIBCFS_FREE(buf, data->ioc_plen1);
                return -EFAULT;
        }

        mutex_down(&console_session.ses_mutex);

        console_session.ses_laststamp = cfs_time_current_sec();

        if (console_session.ses_shutdown) {
                rc = -ESHUTDOWN;
                goto out;
        }

        if (console_session.ses_expired)
                lstcon_session_end();

        if (opc != LSTIO_SESSION_NEW &&
            console_session.ses_state == LST_SESSION_NONE) {
                CDEBUG(D_NET, "LST no active session\n");
                rc = -ESRCH;
                goto out;
        }

        memset(&console_session.ses_trans_stat, 0, sizeof(lstcon_trans_stat_t));
        
        switch (opc) {
                case LSTIO_SESSION_NEW:
                        rc = lst_session_new_ioctl((lstio_session_new_args_t *)buf);
                        break;
                case LSTIO_SESSION_END:
                        rc = lst_session_end_ioctl((lstio_session_end_args_t *)buf);
                        break;
                case LSTIO_SESSION_INFO:
                        rc = lst_session_info_ioctl((lstio_session_info_args_t *)buf);
                        break;
                case LSTIO_DEBUG:
                        rc = lst_debug_ioctl((lstio_debug_args_t *)buf);
                        break;
                case LSTIO_GROUP_ADD:
                        rc = lst_group_add_ioctl((lstio_group_add_args_t *)buf);
                        break;
                case LSTIO_GROUP_DEL:
                        rc = lst_group_del_ioctl((lstio_group_del_args_t *)buf);
                        break;
                case LSTIO_GROUP_UPDATE:
                        rc = lst_group_update_ioctl((lstio_group_update_args_t *)buf);
                        break;
                case LSTIO_NODES_ADD:
                        rc = lst_nodes_add_ioctl((lstio_group_nodes_args_t *)buf);
                        break;
                case LSTIO_GROUP_LIST:
                        rc = lst_group_list_ioctl((lstio_group_list_args_t *)buf);
                        break;
                case LSTIO_GROUP_INFO:
                        rc = lst_group_info_ioctl((lstio_group_info_args_t *)buf);
                        break;
                case LSTIO_BATCH_ADD:
                        rc = lst_batch_add_ioctl((lstio_batch_add_args_t *)buf);
                        break;
                case LSTIO_BATCH_START:
                        rc = lst_batch_run_ioctl((lstio_batch_run_args_t *)buf);
                        break;
                case LSTIO_BATCH_STOP:
                        rc = lst_batch_stop_ioctl((lstio_batch_stop_args_t *)buf);
                        break;
                case LSTIO_BATCH_QUERY:
                        rc = lst_batch_query_ioctl((lstio_batch_query_args_t *)buf);
                        break;
                case LSTIO_BATCH_LIST:
                        rc = lst_batch_list_ioctl((lstio_batch_list_args_t *)buf);
                        break;
                case LSTIO_BATCH_INFO:
                        rc = lst_batch_info_ioctl((lstio_batch_info_args_t *)buf);
                        break;
                case LSTIO_TEST_ADD:
                        rc = lst_test_add_ioctl((lstio_test_args_t *)buf);
                        break;
                case LSTIO_STAT_QUERY:
                        rc = lst_stat_query_ioctl((lstio_stat_args_t *)buf);
                        break;
                default:
                        rc = -EINVAL;
        }

        if (copy_to_user(data->ioc_pbuf2, &console_session.ses_trans_stat,
                         sizeof(lstcon_trans_stat_t)))
                rc = -EFAULT;
out:
        mutex_up(&console_session.ses_mutex);

        LIBCFS_FREE(buf, data->ioc_plen1);

        return rc;
}

EXPORT_SYMBOL(lstcon_ioctl_entry);

#endif
