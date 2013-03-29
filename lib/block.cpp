// Copyright (C) by Josh Blum. See LICENSE.txt for licensing information.

#include "element_impl.hpp"
#include <gras/block.hpp>
#include <boost/foreach.hpp>
#include <boost/thread/thread.hpp> //yield

using namespace gras;

InputPortConfig::InputPortConfig(void)
{
    item_size = 1;
    reserve_items = 1;
    maximum_items = 0;
    inline_buffer = false;
    preload_items = 0;
}

OutputPortConfig::OutputPortConfig(void)
{
    item_size = 1;
    reserve_items = 1;
    maximum_items = 0;
}

Block::Block(void)
{
    //NOP
}

Block::Block(const std::string &name):
    Element(name)
{
    (*this)->block.reset(new BlockActor());
    (*this)->thread_pool = (*this)->block->thread_pool; //ref copy of pool
    (*this)->block->name = name; //for debug purposes

    //setup some state variables
    (*this)->block->block_ptr = this;
    (*this)->block->block_state = BlockActor::BLOCK_STATE_INIT;

    //call block methods to init stuff
    this->input_config(0) = InputPortConfig();
    this->output_config(0) = OutputPortConfig();
    this->set_interruptible_work(false);
    this->set_buffer_affinity(-1);
}

Block::~Block(void)
{
    //NOP
}

void ElementImpl::block_cleanup(void)
{
    //wait for actor to chew through enqueued messages
    while (this->block->GetNumQueuedMessages())
    {
        //TODO timeout if this does not stop
        boost::this_thread::yield();
    }

    //delete the actor
    this->block.reset();

    //unref actor's framework
    this->thread_pool.reset(); //must be deleted after actor
}

template <typename V>
const typename V::value_type &vector_get_const(const V &v, const size_t index)
{
    if (v.size() <= index)
    {
        return v.back();
    }
    return v[index];
}

template <typename V>
typename V::value_type &vector_get_resize(V &v, const size_t index)
{
    if (v.size() <= index)
    {
        if (v.empty()) v.resize(1);
        v.resize(index+1, v.back());
    }
    return v[index];
}

InputPortConfig &Block::input_config(const size_t which_input)
{
    return vector_get_resize((*this)->block->input_configs, which_input);
}

const InputPortConfig &Block::input_config(const size_t which_input) const
{
    return vector_get_const((*this)->block->input_configs, which_input);
}

OutputPortConfig &Block::output_config(const size_t which_output)
{
    return vector_get_resize((*this)->block->output_configs, which_output);
}

const OutputPortConfig &Block::output_config(const size_t which_output) const
{
    return vector_get_const((*this)->block->output_configs, which_output);
}

void Block::commit_config(void)
{
    for (size_t i = 0; i < (*this)->block->get_num_inputs(); i++)
    {
        InputUpdateMessage message;
        message.index = i;
        (*this)->block->Push(message, Theron::Address());
    }
    for (size_t i = 0; i < (*this)->block->get_num_outputs(); i++)
    {
        OutputUpdateMessage message;
        message.index = i;
        (*this)->block->Push(message, Theron::Address());
    }

}

void Block::consume(const size_t which_input, const size_t num_items)
{
    ASSERT(long(num_items) >= 0); //sign bit set? you dont want a negative
    (*this)->block->consume(which_input, num_items);
}

void Block::produce(const size_t which_output, const size_t num_items)
{
    ASSERT(long(num_items) >= 0); //sign bit set? you dont want a negative
    (*this)->block->produce(which_output, num_items);
}

void Block::consume(const size_t num_items)
{
    const size_t num_inputs = (*this)->block->get_num_inputs();
    for (size_t i = 0; i < num_inputs; i++)
    {
        (*this)->block->consume(i, num_items);
    }
}

void Block::produce(const size_t num_items)
{
    const size_t num_outputs = (*this)->block->get_num_outputs();
    for (size_t o = 0; o < num_outputs; o++)
    {
        (*this)->block->produce(o, num_items);
    }
}

item_index_t Block::get_consumed(const size_t which_input)
{
    return (*this)->block->stats.items_consumed[which_input];
}

item_index_t Block::get_produced(const size_t which_output)
{
    return (*this)->block->stats.items_produced[which_output];
}

void Block::post_output_tag(const size_t which_output, const Tag &tag)
{
    (*this)->block->stats.tags_produced[which_output]++;
    (*this)->block->post_downstream(which_output, InputTagMessage(tag));
}

TagIter Block::get_input_tags(const size_t which_input)
{
    const std::vector<Tag> &input_tags = (*this)->block->input_tags[which_input];
    return TagIter(input_tags.begin(), input_tags.end());
}

void Block::post_output_msg(const size_t which_output, const PMCC &msg)
{
    (*this)->block->stats.msgs_produced[which_output]++;
    (*this)->block->post_downstream(which_output, InputMsgMessage(msg));
}

PMCC Block::pop_input_msg(const size_t which_input)
{
    std::vector<PMCC> &input_msgs = (*this)->block->input_msgs[which_input];
    size_t &num_read = (*this)->block->num_input_msgs_read[which_input];
    if (num_read >= input_msgs.size()) return PMCC();
    PMCC p = input_msgs[num_read++];
    (*this)->block->stats.msgs_consumed[which_input]++;
    return p;
}

void Block::propagate_tags(const size_t i, const TagIter &iter)
{
    const size_t num_outputs = (*this)->block->get_num_outputs();
    for (size_t o = 0; o < num_outputs; o++)
    {
        BOOST_FOREACH(gras::Tag t, iter)
        {
            t.offset -= this->get_consumed(i);
            t.offset += this->get_produced(o);
            this->post_output_tag(o, t);
        }
    }
}

void Block::notify_active(void)
{
    //NOP
}

void Block::notify_inactive(void)
{
    //NOP
}

void Block::notify_topology(const size_t, const size_t)
{
    return;
}

void Block::set_buffer_affinity(const long affinity)
{
    (*this)->block->buffer_affinity = affinity;
}

void Block::set_interruptible_work(const bool enb)
{
    (*this)->block->interruptible_work = enb;
}

void Block::mark_output_fail(const size_t which_output)
{
    (*this)->block->output_fail(which_output);
}

void Block::mark_input_fail(const size_t which_input)
{
    (*this)->block->input_fail(which_input);
}

void Block::mark_done(void)
{
    (*this)->block->mark_done();
}

SBuffer Block::get_input_buffer(const size_t which_input) const
{
    return (*this)->block->input_queues.front(which_input);
}

SBuffer Block::get_output_buffer(const size_t which_output) const
{
    SBuffer &buff = (*this)->block->output_queues.front(which_output);
    //increment length to auto pop full buffer size,
    //when user doesnt call pop_output_buffer()
    buff.length = buff.get_actual_length();
    return buff;
}

void Block::pop_output_buffer(const size_t which_output, const size_t num_bytes)
{
    (*this)->block->output_queues.front(which_output).length = num_bytes;
}

void Block::post_output_buffer(const size_t which_output, const SBuffer &buffer)
{
    (*this)->block->produce_buffer(which_output, buffer);
}
