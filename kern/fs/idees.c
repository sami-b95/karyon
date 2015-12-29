
struct block
{
	ui32_t in_use;
	ui32_t n;
	bool_t dirty;
	struct block *prev_dirty, *next_dirty;
	ui8_t data[512];
};

struct block *first_dirty = 0;

//////////////////////////////////////////////

struct block *block_ptr = &block_table[offset];

if(block_ptr->in_use && block_ptr->dirty)
{
	ata_write_block(block_ptr->n, block_ptr->data);

	if(first_dirty == block_ptr)
	{
		if(block_ptr->next_dirty == block_ptr)
		{
			first_dirty = 0;
		}
		else
		{
			first_dirty = block_ptr->next_dirty;
		}
	}

	block_ptr->prev_dirty->next_dirty = block_ptr->next_dirty;
	block_ptr->next_dirty->prev_dirty = block_ptr->prev_dirty;
	block_ptr->next_dirty = block_ptr->prev_dirty = 0;
}

block_ptr->in_use = 1;
block_ptr->n = n;
block_ptr->dirty = 0;

cache_put_block(), cache_read_block(), cache_write_block()
