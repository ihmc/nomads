#include "BloomFilter.h"

using namespace NOMADSUtil;

BloomFilter::BloomFilter (void)
    : bit_table_ (0),
      salt_count_ (0),
      table_size_ (0),
      raw_table_size_ (0),
      projected_element_count_ (0),
      inserted_element_count_ (0),
      random_seed_ (0),
      desired_false_positive_probability_ (0.0)
{
}

BloomFilter::BloomFilter (const BloomParameters& p)
    : bit_table_ (0),
      projected_element_count_ (p.projected_element_count),
      inserted_element_count_ (0),
      random_seed_ ((p.random_seed * 0xA5A5A5A5) + 1),
      desired_false_positive_probability_ (p.false_positive_probability)
{
    salt_count_ = p.optimal_parameters.number_of_hashes;
    table_size_ = p.optimal_parameters.table_size;
    generate_unique_salt ();
    raw_table_size_ = table_size_ / bits_per_char;
    bit_table_ = new cell_type[static_cast<std::size_t>(raw_table_size_)];
    std::fill_n (bit_table_, raw_table_size_, 0x00);
}

BloomFilter::BloomFilter (const BloomFilter& filter)
{
    this->operator=(filter);
}

BloomFilter::~BloomFilter (void)
{
    delete[] bit_table_;
}
