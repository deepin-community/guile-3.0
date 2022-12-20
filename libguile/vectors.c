/* Copyright 1995-1996,1998-2001,2006,2008-2012,2014,2018-2020
     Free Software Foundation, Inc.

   This file is part of Guile.

   Guile is free software: you can redistribute it and/or modify it
   under the terms of the GNU Lesser General Public License as published
   by the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Guile is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
   License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with Guile.  If not, see
   <https://www.gnu.org/licenses/>.  */




#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "array-handle.h"
#include "bdw-gc.h"
#include "boolean.h"
#include "eq.h"
#include "gsubr.h"
#include "list.h"
#include "numbers.h"
#include "pairs.h"
#include "vectors.h"

#include "generalized-vectors.h"




#define VECTOR_MAX_LENGTH (SCM_T_BITS_MAX >> 8)

#define SCM_VALIDATE_MUTABLE_VECTOR(pos, v)                             \
  do {                                                                  \
    SCM_ASSERT_TYPE (SCM_I_IS_MUTABLE_VECTOR (v), v, pos, FUNC_NAME,    \
                     "mutable vector");                                 \
  } while (0)


int
scm_is_vector (SCM obj)
{
  return SCM_I_IS_VECTOR (obj);
}

int
scm_is_simple_vector (SCM obj)
{
  return SCM_I_IS_VECTOR (obj);
}

const SCM *
scm_vector_elements (SCM vec, scm_t_array_handle *h,
		     size_t *lenp, ssize_t *incp)
{
  scm_array_get_handle (vec, h);
  if (1 != scm_array_handle_rank (h))
    {
      scm_array_handle_release (h);
      scm_wrong_type_arg_msg (NULL, 0, vec, "rank 1 array of Scheme values");
    }
  
  if (lenp)
    {
      scm_t_array_dim *dim = scm_array_handle_dims (h);
      *lenp = dim->ubnd - dim->lbnd + 1;
      *incp = dim->inc;
    }
  return scm_array_handle_elements (h);
}

SCM *
scm_vector_writable_elements (SCM vec, scm_t_array_handle *h,
			      size_t *lenp, ssize_t *incp)
{
  const SCM *ret = scm_vector_elements (vec, h, lenp, incp);

  if (h->writable_elements != h->elements)
    scm_wrong_type_arg_msg (NULL, 0, vec, "mutable vector");

  return (SCM *) ret;
}

SCM_DEFINE (scm_vector_p, "vector?", 1, 0, 0, 
	    (SCM obj),
	    "Return @code{#t} if @var{obj} is a vector, otherwise return\n"
	    "@code{#f}.")
#define FUNC_NAME s_scm_vector_p
{
  return scm_from_bool (scm_is_vector (obj));
}
#undef FUNC_NAME

SCM_DEFINE (scm_vector_length, "vector-length", 1, 0, 0, 
	    (SCM v),
            "Returns the number of elements in @var{vector} as an exact integer.")
#define FUNC_NAME s_scm_vector_length
{
  return scm_from_size_t (scm_c_vector_length (v));
}
#undef FUNC_NAME

size_t
scm_c_vector_length (SCM v)
#define FUNC_NAME s_scm_vector_length
{
  SCM_VALIDATE_VECTOR (1, v);

  return SCM_I_VECTOR_LENGTH (v);
}
#undef FUNC_NAME

SCM_REGISTER_PROC (s_list_to_vector, "list->vector", 1, 0, 0, scm_vector);
/*
	    "Return a newly created vector initialized to the elements of"
	    "the list @var{list}.\n\n"
	    "@lisp\n"
	    "(vector->list '#(dah dah didah)) @result{} (dah dah didah)\n"
	    "(list->vector '(dididit dah)) @result{}   #(dididit dah)\n"
	    "@end lisp")
*/
SCM_DEFINE (scm_vector, "vector", 0, 0, 1, 
	    (SCM l),
	    "@deffnx {Scheme Procedure} list->vector l\n"
	    "Return a newly allocated vector composed of the\n"
	    "given arguments.  Analogous to @code{list}.\n"
	    "\n"
	    "@lisp\n"
	    "(vector 'a 'b 'c) @result{} #(a b c)\n"
	    "@end lisp")
#define FUNC_NAME s_scm_vector
{
  SCM res;
  SCM *data;
  long i, len;

  SCM_VALIDATE_LIST_COPYLEN (1, l, len);

  res = scm_c_make_vector (len, SCM_UNSPECIFIED);
  data = SCM_I_VECTOR_WELTS (res);
  i = 0;
  while (scm_is_pair (l) && i < len) 
    {
      data[i] = SCM_CAR (l);
      l = SCM_CDR (l);
      i += 1;
    }

  return res;
}
#undef FUNC_NAME

SCM_DEFINE (scm_vector_ref, "vector-ref", 2, 0, 0, 
	    (SCM vector, SCM k),
            "@var{k} must be a valid index of @var{vector}.\n"
            "@samp{Vector-ref} returns the contents of element @var{k} of\n"
            "@var{vector}.\n\n"
            "@lisp\n"
            "(vector-ref '#(1 1 2 3 5 8 13 21) 5) @result{} 8\n"
            "(vector-ref '#(1 1 2 3 5 8 13 21)\n"
            "    (let ((i (round (* 2 (acos -1)))))\n"
            "      (if (inexact? i)\n"
            "        (inexact->exact i)\n"
            "           i))) @result{} 13\n"
            "@end lisp")
#define FUNC_NAME s_scm_vector_ref
{
  return scm_c_vector_ref (vector, scm_to_size_t (k));
}
#undef FUNC_NAME

SCM
scm_c_vector_ref (SCM v, size_t k)
#define FUNC_NAME s_scm_vector_ref
{
  SCM_VALIDATE_VECTOR (1, v);

  if (k >= SCM_I_VECTOR_LENGTH (v))
    scm_out_of_range (NULL, scm_from_size_t (k));

  return SCM_SIMPLE_VECTOR_REF (v, k);
}
#undef FUNC_NAME

SCM_DEFINE (scm_vector_set_x, "vector-set!", 3, 0, 0, 
	    (SCM vector, SCM k, SCM obj),
            "@var{k} must be a valid index of @var{vector}.\n"
            "@code{Vector-set!} stores @var{obj} in element @var{k} of @var{vector}.\n"
            "The value returned by @samp{vector-set!} is unspecified.\n"
            "@lisp\n"
            "(let ((vec (vector 0 '(2 2 2 2) \"Anna\")))\n"
            "  (vector-set! vec 1 '(\"Sue\" \"Sue\"))\n"
            "  vec) @result{}  #(0 (\"Sue\" \"Sue\") \"Anna\")\n"
            "(vector-set! '#(0 1 2) 1 \"doe\") @result{} @emph{error} ; constant vector\n"
            "@end lisp")
#define FUNC_NAME s_scm_vector_set_x
{
  scm_c_vector_set_x (vector, scm_to_size_t (k), obj);
  return SCM_UNSPECIFIED;
}
#undef FUNC_NAME

void
scm_c_vector_set_x (SCM v, size_t k, SCM obj)
#define FUNC_NAME s_scm_vector_set_x
{
  SCM_VALIDATE_MUTABLE_VECTOR (1, v);

  if (k >= SCM_I_VECTOR_LENGTH (v))
    scm_out_of_range (NULL, scm_from_size_t (k)); 

  SCM_SIMPLE_VECTOR_SET (v, k, obj);
}
#undef FUNC_NAME

SCM_DEFINE (scm_make_vector, "make-vector", 1, 1, 0,
            (SCM k, SCM fill),
	    "Return a newly allocated vector of @var{k} elements.  If a\n"
	    "second argument is given, then each position is initialized to\n"
	    "@var{fill}.  Otherwise the initial contents of each position is\n"
	    "unspecified.")
#define FUNC_NAME s_scm_make_vector
{
  size_t l = scm_to_unsigned_integer (k, 0, VECTOR_MAX_LENGTH);

  if (SCM_UNBNDP (fill))
    fill = SCM_UNSPECIFIED;
  
  return scm_c_make_vector (l, fill);
}
#undef FUNC_NAME

static SCM
make_vector (size_t size)
{
  return scm_words ((size << 8) | scm_tc7_vector, size + 1);
}

SCM
scm_c_make_vector (size_t k, SCM fill)
#define FUNC_NAME s_scm_make_vector
{
  SCM vector;
  size_t j;

  SCM_ASSERT_RANGE (1, scm_from_size_t (k), k <= VECTOR_MAX_LENGTH);

  vector = make_vector (k);
  for (j = 0; j < k; ++j)
    SCM_SIMPLE_VECTOR_SET (vector, j, fill);

  return vector;
}
#undef FUNC_NAME

SCM_DEFINE (scm_vector_copy, "vector-copy", 1, 0, 0,
	    (SCM vec),
	    "Return a copy of @var{vec}.")
#define FUNC_NAME s_scm_vector_copy
{
  scm_t_array_handle handle;
  size_t i, len;
  ssize_t inc;
  const SCM *src;
  SCM result, *dst;

  src = scm_vector_elements (vec, &handle, &len, &inc);

  result = make_vector (len);
  dst = SCM_I_VECTOR_WELTS (result);
  for (i = 0; i < len; i++, src += inc)
    dst[i] = *src;

  scm_array_handle_release (&handle);

  return result;
}
#undef FUNC_NAME


SCM_DEFINE (scm_vector_to_list, "vector->list", 1, 0, 0, 
	    (SCM v),
	    "Return a newly allocated list composed of the elements of @var{v}.\n"
	    "\n"
	    "@lisp\n"
	    "(vector->list '#(dah dah didah)) @result{}  (dah dah didah)\n"
	    "(list->vector '(dididit dah)) @result{}  #(dididit dah)\n"
	    "@end lisp")
#define FUNC_NAME s_scm_vector_to_list
{
  SCM res = SCM_EOL;
  const SCM *data;
  scm_t_array_handle handle;
  size_t i, count, len;
  ssize_t inc;

  data = scm_vector_elements (v, &handle, &len, &inc);
  for (i = (len - 1) * inc, count = 0;
       count < len;
       i -= inc, count++)
    res = scm_cons (data[i], res);

  scm_array_handle_release (&handle);
  return res;
}
#undef FUNC_NAME

static SCM scm_vector_fill_partial_x (SCM vec, SCM fill, SCM start, SCM end);

SCM_DEFINE_STATIC (scm_vector_fill_partial_x, "vector-fill!", 2, 2, 0,
            (SCM vec, SCM fill, SCM start, SCM end),
            "Assign the value of every location in vector @var{vec} between\n"
            "@var{start} and @var{end} to @var{fill}.  @var{start} defaults\n"
            "to 0 and @var{end} defaults to the length of @var{vec}.  The value\n"
            "returned by @code{vector-fill!} is unspecified.")
#define FUNC_NAME s_scm_vector_fill_partial_x
{
  SCM_VALIDATE_MUTABLE_VECTOR(1, vec);

  SCM *data;
  size_t i = 0;
  size_t len = SCM_I_VECTOR_LENGTH (vec);

  data = SCM_I_VECTOR_WELTS (vec);

  if (!SCM_UNBNDP (start))
    i = scm_to_unsigned_integer (start, 0, len);

  if (!SCM_UNBNDP (end))
    len = scm_to_unsigned_integer (end, i, len);

  for (; i < len; ++i)
    data[i] = fill;

  return SCM_UNSPECIFIED;
}
#undef FUNC_NAME


SCM
scm_vector_fill_x (SCM vec, SCM fill)
#define FUNC_NAME s_scm_vector_fill_x
{
  return scm_vector_fill_partial_x (vec, fill, SCM_UNDEFINED, SCM_UNDEFINED);
}
#undef FUNC_NAME


SCM
scm_i_vector_equal_p (SCM x, SCM y)
{
  long i;
  for (i = SCM_I_VECTOR_LENGTH (x) - 1; i >= 0; i--)
    if (scm_is_false (scm_equal_p (SCM_I_VECTOR_ELTS (x)[i],
				   SCM_I_VECTOR_ELTS (y)[i])))
      return SCM_BOOL_F;
  return SCM_BOOL_T;
}


SCM_DEFINE (scm_vector_move_left_x, "vector-move-left!", 5, 0, 0, 
            (SCM vec1, SCM start1, SCM end1, SCM vec2, SCM start2),
	    "Copy elements from @var{vec1}, positions @var{start1} to @var{end1},\n"
	    "to @var{vec2} starting at position @var{start2}.  @var{start1} and\n"
	    "@var{start2} are inclusive indices; @var{end1} is exclusive.\n\n"
	    "@code{vector-move-left!} copies elements in leftmost order.\n"
	    "Therefore, in the case where @var{vec1} and @var{vec2} refer to the\n"
	    "same vector, @code{vector-move-left!} is usually appropriate when\n"
	    "@var{start1} is greater than @var{start2}.")
#define FUNC_NAME s_scm_vector_move_left_x
{
  scm_t_array_handle handle1, handle2;
  const SCM *elts1;
  SCM *elts2;
  size_t len1, len2;
  ssize_t inc1, inc2;
  size_t i, j, e;
  
  elts1 = scm_vector_elements (vec1, &handle1, &len1, &inc1);
  elts2 = scm_vector_writable_elements (vec2, &handle2, &len2, &inc2);

  i = scm_to_unsigned_integer (start1, 0, len1);
  e = scm_to_unsigned_integer (end1, i, len1);
  SCM_ASSERT_RANGE (SCM_ARG3, end1, (e-i) <= len2);
  j = scm_to_unsigned_integer (start2, 0, len2);
  SCM_ASSERT_RANGE (SCM_ARG5, start2, j <= len2 - (e - i));
  
  i *= inc1;
  e *= inc1;
  j *= inc2;
  for (; i < e; i += inc1, j += inc2)
    elts2[j] = elts1[i];

  scm_array_handle_release (&handle2);
  scm_array_handle_release (&handle1);

  return SCM_UNSPECIFIED;
}
#undef FUNC_NAME

SCM_DEFINE (scm_vector_move_right_x, "vector-move-right!", 5, 0, 0, 
            (SCM vec1, SCM start1, SCM end1, SCM vec2, SCM start2),
	    "Copy elements from @var{vec1}, positions @var{start1} to @var{end1},\n"
	    "to @var{vec2} starting at position @var{start2}.  @var{start1} and\n"
	    "@var{start2} are inclusive indices; @var{end1} is exclusive.\n\n"
	    "@code{vector-move-right!} copies elements in rightmost order.\n"
	    "Therefore, in the case where @var{vec1} and @var{vec2} refer to the\n"
	    "same vector, @code{vector-move-right!} is usually appropriate when\n"
	    "@var{start1} is less than @var{start2}.")
#define FUNC_NAME s_scm_vector_move_right_x
{
  scm_t_array_handle handle1, handle2;
  const SCM *elts1;
  SCM *elts2;
  size_t len1, len2;
  ssize_t inc1, inc2;
  size_t i, j, e;
  
  elts1 = scm_vector_elements (vec1, &handle1, &len1, &inc1);
  elts2 = scm_vector_writable_elements (vec2, &handle2, &len2, &inc2);

  i = scm_to_unsigned_integer (start1, 0, len1);
  e = scm_to_unsigned_integer (end1, i, len1);
  SCM_ASSERT_RANGE (SCM_ARG3, end1, (e-i) <= len2);
  j = scm_to_unsigned_integer (start2, 0, len2);
  SCM_ASSERT_RANGE (SCM_ARG5, start2, j <= len2 - (e - i));
  
  j += (e - i);
  
  i *= inc1;
  e *= inc1;
  j *= inc2;
  while (i < e)
    {
      e -= inc1;
      j -= inc2;
      elts2[j] = elts1[e];
    }

  scm_array_handle_release (&handle2);
  scm_array_handle_release (&handle1);

  return SCM_UNSPECIFIED;
}
#undef FUNC_NAME


SCM_VECTOR_IMPLEMENTATION (SCM_ARRAY_ELEMENT_TYPE_SCM, scm_make_vector)


void
scm_init_vectors ()
{
#include "vectors.x"
}

