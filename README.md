# Parallel Bloom Filter Implementation with OpenMP
This repository contains an implementation of a Bloom filter in C, utilizing OpenMP for shared memory data parallelism. The Bloom filter is a space-efficient probabilistic data structure that is used to test whether an element is a member of a set. This parallel implementation aims to improve the performance of the Bloom filter by leveraging multiple threads through OpenMP.

## Getting Started
These instructions will help you get a copy of the project up and running on your local machine for development and testing purposes. See deployment for notes on how to deploy the project on a live system.

## Prerequisites
To compile and run this project, you will need a C compiler that supports OpenMP, such as GCC or Clang.

## Usage
The Makefile can be utilized to compile both the serial and parallel implementations of the Bloom filter.
The program arguments are the word and query files and can be used as follows:
./bloom words.txt query.txt
