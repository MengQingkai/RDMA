#include <stdio.h>
#include <stdlib.h>
#include <infiniband/verbs.h>

int main()
{
    struct ibv_device **device_list;
    int num_devices;
    int i;

    device_list = ibv_get_device_list(&num_devices);
    if (!device_list) {
        fprintf(stderr, "Error, ibv_get_device_list() failed\n");
        exit(1);
    }
    else{ printf("num_devices: %d\n ",device_list);}

    for (i = 0; i < num_devices; ++i)
    {
        struct ibv_context * ibContext;
        ibContext = ibv_open_device(device_list[i]);

        printf("RDMA device[%d]: name %s, node_type: %s, transport_type: %d, GUID: %d \n", i, ibv_get_device_name(device_list[i]), \
                ibv_node_type_str(device_list[i]->node_type), device_list[i]->transport_type, ibv_get_device_guid(device_list[i]));

        printf("Num of completions vectors this device support : %d\n", ibContext->num_comp_vectors);
        printf("File descriptor that will be used to report about asynchronic events: %d", ibContext->async_fd);

        struct ibv_device_attr device_attr;
        if(ibv_query_device(ibContext, &device_attr)) {
            fprintf(stderr, "Error, failed to query device '%s' \n", ibv_get_device_name(ibContext->device));
            return -1;
        }

        struct ibv_pd *pd;
        pd = ibv_alloc_pd(ibContext);
        if (!pd) {
            fprintf(stderr, "Error, ibv_alloc_pd() failed\n");
            return -1;
        }

        void * buf = malloc(sizeof(char) * 100);
        struct ibv_mr * mr = ibv_reg_mr(pd, buf, sizeof(char) * 100, IBV_ACCESS_LOCAL_WRITE);
        if(!mr) {
            fprintf(stderr, "Error, ibv_reg_mr() failed \n");
            return -1;
        }

        struct ibv_comp_channel* comp_channel = ibv_create_comp_channel(ibContext);
        if(!comp_channel) {
            fprintf(stderr, "Error, ibv_create_comp_channel() failed\n");
            return -1;
        }

        struct ibv_cq *cq1, *cq2;
        int cq_size = 10;
        cq1 = ibv_create_cq(ibContext, cq_size, NULL, comp_channel, 0);
        if (!cq1) {
            fprintf(stderr, "Error, ibv_create_cq() failed\n");
            return -1;
        }

        cq2 = ibv_create_cq(ibContext, cq_size, NULL, NULL, 0);
        if (!cq2) {
            fprintf(stderr, "Error, ibv_create_cq() failed\n");
            return -1;
        }

        struct ibv_qp *qp;
        struct ibv_qp_init_attr init_attr = {
            .send_cq = cq1,
            .recv_cq = cq1,
            .cap = {
                .max_send_wr = 1,
                .max_recv_wr = 3,
                .max_send_sge = 1,
                .max_recv_sge = 1
            },
            .qp_type = IBV_QPT_RC
        };

        qp = ibv_create_qp(pd, &init_attr);
        if (!qp) {
            fprintf(stderr, "Error, ibv_create_qp() failed\n");
            return -1;
        }



        if (ibv_destroy_qp(qp)) {
            fprintf(stderr, "Error, ibv_destroy_qp() failed\n");
            return -1;
        }



        if (ibv_destroy_cq(cq2)) {
            fprintf(stderr, "Errory ibv_destroy_cq(cq2) failed \n");
            return -1;
        }

        if (ibv_destroy_cq(cq1)) {
            fprintf(stderr, "Error, ibv_destroy_cq(cq1) failed \n");
            return -1;
        }

        if (ibv_destroy_comp_channel(comp_channel)) {
            fprintf(stderr, "Error, ibv_destroy_comp_channel() failed\n");
            return -1;
        }

        if( ibv_dereg_mr(mr) ){
            fprintf(stderr, "Error, ibv_dereg_mr() failed\n");
            return -1;
        }


        if( ibv_dealloc_pd(pd) ) {
            fprintf(stderr, "Error, ibv_dealloc_pd() failed\n");
            return -1;
        }


        ibv_close_device(ibContext);
    }

    ibv_free_device_list(device_list);
}
