// Copyright (C) by Josh Blum. See LICENSE.txt for licensing information.

#ifndef INCLUDED_GRAS_SBUFFER_I
#define INCLUDED_GRAS_SBUFFER_I

%{
#include <gras/gras.hpp>
#include <gras/sbuffer.hpp>
%}

////////////////////////////////////////////////////////////////////////
// remove base class warning
////////////////////////////////////////////////////////////////////////
#pragma SWIG nowarn=401

////////////////////////////////////////////////////////////////////////
// Export swig element comprehension
////////////////////////////////////////////////////////////////////////
%include <gras/gras.hpp>
%include <gras/sbuffer.hpp>

////////////////////////////////////////////////////////////////////////
// Replace the get method
////////////////////////////////////////////////////////////////////////
%extend gras::SBuffer
{
    %insert("python")
    %{
        def get(self):
            from gras.GRAS_Utils import pointer_to_ndarray
            addr = long(self.get_actual_memory())
            return pointer_to_ndarray(
                addr=addr + self.offset,
                nitems=self.length,
            )

        def __len__(self): return self.length

    %}
}

#endif /*INCLUDED_GRAS_SBUFFER_I*/
