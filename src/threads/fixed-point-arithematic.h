#ifndef THREADS_FIXED_POINT_ARITHEMATIC_H
#define THREADS_FIXED_POINT_ARITHEMATIC_H

#define FIXED_PQ 32
#define FIXED_P 17
#define FIXED_Q 14
#define FIXED_F (2 << (FIXED_Q - 1))

typedef struct 
{
    int number;
} fixed_point_t;

static inline fixed_point_t
fix_convert_to_fixedpoint(int n) 
{
    fixed_point_t x;
    x.number = n * FIXED_F;
    return x;
}

static inline int
fix_convert_to_integer(fixed_point_t x)
{
    if (x.number >= 0) x.number += (FIXED_F/2);
    else x.number -= (FIXED_F/2);
    return x.number / FIXED_F;
}

static inline fixed_point_t
fix_add(fixed_point_t x, fixed_point_t y) 
{
    fixed_point_t z;
    z.number = x.number + y.number;
    return z;
}

static inline fixed_point_t
fix_and_add(fixed_point_t x, int n) 
{
    fixed_point_t z;
    z.number = x.number + (n * FIXED_F);
    return z;
}

static inline fixed_point_t
fix_subtract(fixed_point_t x, fixed_point_t y)
{
    fixed_point_t z;
    z.number = x.number - y.number;
    return z;
}

static inline fixed_point_t
fix_and_subtract(fixed_point_t x, int n) 
{
    fixed_point_t z;
    z.number = x.number - (n * FIXED_F);
    return z;
}

static inline fixed_point_t
fix_multiply(fixed_point_t x, fixed_point_t y)
{
    fixed_point_t z;
    z.number = ((int64_t) x.number) * y.number / FIXED_F;
    return z;
}

static inline fixed_point_t
fix_multiply_to(fixed_point_t x, int n)
{
    x.number *= n;
    return x;
}

static inline fixed_point_t
fix_divide(fixed_point_t x, fixed_point_t y)
{
    fixed_point_t z;
    z.number = ((int64_t) x.number) * FIXED_F / y.number;
    return z;
}

static inline fixed_point_t
fix_divide_from(fixed_point_t x, int n)
{
    x.number /= n;
    return x;
}



#endif