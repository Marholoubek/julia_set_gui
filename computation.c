#include "utils.h"
#include "computation.h"

static struct { // Structure for variables we need for computation
    double c_re;
    double c_im;
    int n;

    double range_re_min;
    double range_re_max;
    double range_im_min;
    double range_im_max;

    int grid_w;
    int grid_h;

    int cur_x;
    int cur_y;

    double d_re;
    double d_im;

    int nbr_chunks;
    int cid;
    double chunk_re;
    double chunk_im;

    uint8_t chunk_n_re;
    uint8_t chunk_n_im;

    uint8_t *grid;
    bool computing;
    bool done;
    bool abort;
    bool is_set;


} comp = {
        .c_re = -0.4,
        .c_im = 0.6,
        .n = 60,
        .range_re_min = -1.6,
        .range_re_max = 1.6,
        .range_im_min = -1.1,
        .range_im_max = 1.1,

        .grid_w = 640,
        .grid_h = 480,

        .chunk_n_re = 64,
        .chunk_n_im = 48,

        .grid = NULL,
        .computing = false,
        .done = false,
        .abort = false,
        .is_set = false,

};

void computation_init(void){ // Allocation of memory for computation and computing some variables
    comp.grid = my_alloc(comp.grid_w * comp.grid_h);
    comp.d_re = (comp.range_re_max - comp.range_re_min) / (1. * comp.grid_w);
    comp.d_im = -(comp.range_re_max - comp.range_re_min) / (1. * comp.grid_h);
    comp.nbr_chunks = (comp.grid_w * comp.grid_h) /  (comp.chunk_n_re * comp.chunk_n_im);

}

void computation_cleanup(void){ // Cleaning the memory
    if (comp.grid) free(comp.grid);
    comp.grid = NULL;
}

bool reset_chunk(void){ // Resetting the chunk index
    if (!is_abort()) {
        comp.cid = 0;
        comp.computing = true;
        comp.cur_x = comp.cur_y = 0;
        comp.chunk_re = comp.range_re_min;
        comp.chunk_im = comp.range_im_max;
    }
    return is_abort();
}

/* Informative functions */
bool is_computing(void) {return comp.computing;}
bool is_done(void) {return comp.done;}
bool is_abort(void) {return comp.abort;}
bool is_set(void){ return comp.is_set;}

void abort_comp(void){comp.abort = true;}
void enable_comp(void){comp.abort = false;}

bool set_compute(message *msg){ // Setting the computation values
    my_assert(msg != NULL, __func__ ,__LINE__, __FILE__);
    bool ret = !is_computing();

    if (ret){
        msg->type = MSG_SET_COMPUTE;
        msg->data.set_compute.c_re = comp.c_re;
        msg->data.set_compute.c_im = comp.c_im;
        msg->data.set_compute.d_re = comp.d_re;
        msg->data.set_compute.d_im = comp.d_im;
        msg->data.set_compute.n = comp.n;
        comp.done = false;
        comp.is_set = true;
        printf("Computations parameters: w:%d, h:%d, chunks: %d\n", comp.grid_w, comp.grid_h, comp.nbr_chunks);
        printf("Computations values: c = %.3f + %.3fj, Intervals: %.3f + %.3fj and %.3f + %.3fj,\n", comp.c_re, comp.c_im, comp.range_re_min, comp.range_im_min, comp.range_re_max, comp.range_im_max);
    }

    return ret;
}

bool compute(message *msg){ // Filling the message with data we need to be computed
    my_assert(msg != NULL, __func__ ,__LINE__, __FILE__);
    if (!is_computing()) {
        comp.cid = 0;
        comp.computing = true;
        comp.done = false;
        comp.cur_x = comp.cur_y = 0;
        comp.chunk_re = comp.range_re_min;
        comp.chunk_im = comp.range_im_max;
        msg->type = MSG_COMPUTE;
    } else {
        comp.cid += 1;
        if (comp.cid < comp.nbr_chunks){
            comp.cur_x += comp.chunk_n_re;
            comp.chunk_re += comp.chunk_n_re * comp.d_re;
            if (comp.cur_x >= comp.grid_w) {
                comp.cur_x = 0;
                comp.cur_y += comp.chunk_n_im;
                comp.chunk_re = comp.range_re_min;
                comp.chunk_im += comp.chunk_n_im * comp.d_im;
            }
            msg->type = MSG_COMPUTE;
        } else {
            comp.computing = false;
            comp.done = true;
            comp.abort = false;
        }
    }

    if (comp.computing && msg->type == MSG_COMPUTE){
        msg->data.compute.re = comp.chunk_re;
        msg->data.compute.im = comp.chunk_im;
        msg->data.compute.cid = comp.cid;
        msg->data.compute.n_re = comp.chunk_n_re;
        msg->data.compute.n_im = comp.chunk_n_im;
    }

    return is_computing();
}

void move_chunk_back(void){
    if (comp.cid > 0 && comp.cid < comp.nbr_chunks){
        comp.cid -= 1;
        comp.cur_x -= comp.chunk_n_re;
        comp.chunk_re -= comp.chunk_n_re * comp.d_re;
    }
}

void update_data(const msg_compute_data *compute_data){ // Updating the data in grid
    if (compute_data->cid == comp.cid){
        const int idx = comp.cur_x + compute_data->i_re + (comp.cur_y + compute_data->i_im) * comp.grid_w;
        if (idx >= 0 && idx < (comp.grid_w * comp.grid_h)){
            comp.grid[idx] = compute_data->iter;
        }
        if ((comp.cid + 1) >= comp.nbr_chunks && (compute_data->i_re + 1) == comp.chunk_n_re && (compute_data->i_im + 1) == comp.chunk_n_im){
            comp.done = true;
            comp.computing = false;
        }
    } else error("Received chunk with unexpected chunk id");

}

void get_grid_size(int *w, int *h){
    *w = comp.grid_w;
    *h = comp.grid_h;
}


/* According to grid values, we translate it into colors  */
void redraw(int w, int h, unsigned char *img) {
    uint8_t *grid = comp.grid;
    int threshold = comp.n;
    int nsize = w * h;
    unsigned char *cur = img;
    for (int i = 0; i < nsize; ++i) {
        const int n = *(grid++);
        const double t = 1. * n / threshold;
        if (t < threshold) {
            *(cur++) = (int)(9 * (1 - t) * t * t * t * 255);
            *(cur++) = (int)(15 * (1 - t) * (1 - t) * t * t * 255);
            *(cur++) = (int)(8.5 * (1 - t) * (1 - t) * (1 - t) * t * 255);
        }
        else {
            for (int j = 0; j < 3; ++j) {
                *(cur++) = 0;
            }
        }
    }
}

void buffer_cleanup(void){ // Cleaning the buffer and setting all the values to zero
    computation_cleanup();
    computation_init();
    int w = comp.grid_w;
    int h = comp.grid_h;
    int i = 0;
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            comp.grid[i++] = 0;
        }
    }
}

void my_compute(void){
    complex double z, c;
    int x, y, i;
    int w = comp.grid_w;
    int h = comp.grid_h;
    int n = comp.n;
    c = comp.c_re + comp.c_im * I;
    double range_re = -comp.range_re_min + comp.range_re_max;
    double range_im = -comp.range_im_min + comp.range_im_max;

    comp.computing = true;
    for (y = 0; y < h; ++y) {
        for (x = 0; x < w; ++x) {
            z = (comp.range_re_min + x * (range_re / w)) + (comp.range_im_min + y * (range_im / h)) * I;
            i = 0;
            while (cabs(z) < 2 && ++i < n) z = z * z + c;
            comp.grid[((h - 1 - y) * w) + x] = i;
        }
    }
    comp.computing = false;
}

/* Setting new parameters for computation */
void set_parameters(double c_re, double c_im, double r_re_min, double r_im_min, double r_re_max, double r_im_max){
    if (!is_computing()) {
        comp.c_im = c_im;
        comp.c_re = c_re;
        comp.range_re_min = r_re_min;
        comp.range_im_min = r_im_min;
        comp.range_re_max = r_re_max;
        comp.range_im_max = r_im_max;
    }
}

void zoom(int i){ // Zooming to the point, you're looking
    double range_re = (-comp.range_re_min + comp.range_re_max) / 4;
    double range_im = (-comp.range_im_min + comp.range_im_max) / 4;
    if (comp.range_re_min >= -2 && comp.range_im_min >= -2 && comp.range_re_max <= 2 && comp.range_im_max <= 2) {
        comp.range_re_min += (i)*range_re;
        comp.range_im_min += (i)*range_im;
        comp.range_re_max -= (i)*range_re;
        comp.range_im_max -= (i)*range_im;
    } else {
        comp.range_re_min = -2;
        comp.range_im_min = -2;
        comp.range_re_max = 2;
        comp.range_im_max = 2;
    }
}

void move(char c){ // Mooving leef, right, up and down
    double range_re = (-comp.range_re_min + comp.range_re_max) / 4;
    double range_im = (-comp.range_im_min + comp.range_im_max) / 4;
    switch (c) {
        case 'u':
            comp.range_im_min += range_im;
            comp.range_im_max += range_im;
            break;
        case 'd':
            comp.range_im_min -= range_im;
            comp.range_im_max -= range_im;
            break;
        case 'l':
            comp.range_re_min -= range_re;
            comp.range_re_max -= range_re;
            break;
        case 'r':
            comp.range_re_min += range_re;
            comp.range_re_max += range_re;
            break;
    }
}






