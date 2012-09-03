//
// Copyright 2012 Josh Blum
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with io_sig program.  If not, see <http://www.gnu.org/licenses/>.

%include "gruel_common.i"

%inline %{

namespace gnuradio
{

struct TopBlockPython : TopBlock
{
    TopBlockPython(void):
        TopBlock("top")
    {
        //NOP
    }

    TopBlockPython(const std::string &name):
        TopBlock(name)
    {
        //NOP
    }

    void wait(void)
    {
        GR_PYTHON_BLOCKING_CODE
        (
            TopBlock::wait();
        )
    }
};
}

%}

%pythoncode %{

def internal_connect__(fcn, obj, *args):

    def to_element(obj):
        if isinstance(obj, Element): return obj
        try: return obj.shared_to_element()
        except: raise Exception('cant coerce obj %s to element'%(obj))

    if len(args) == 1:
        fcn(obj, to_element(args[0]))
        return

    for src, sink in zip(args, args[1:]):
        try: src, src_index = src
        except: src_index = 0
        try: sink, sink_index = sink
        except: sink_index = 0
        fcn(obj, to_element(src), src_index, to_element(sink), sink_index)

class top_block(TopBlockPython):
    def __init__(self, *args, **kwargs):
        TopBlockPython.__init__(self, *args, **kwargs)

    def connect(self, *args):
        return internal_connect__(TopBlockPython.connect, self, *args)

    def disconnect(self, *args):
        return internal_connect__(TopBlockPython.disconnect, self, *args)

class hier_block(HierBlock):
    def __init__(self, *args, **kwargs):
        HierBlock.__init__(self, *args, **kwargs)

    def connect(self, *args):
        return internal_connect__(HierBlock.connect, self, *args)

    def disconnect(self, *args):
        return internal_connect__(HierBlock.disconnect, self, *args)

hier_block2 = hier_block

%}