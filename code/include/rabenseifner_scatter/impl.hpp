#pragma once

#include <dsop.h>
#include <mpi.h>

#include <memory>

#include "vector.h"

namespace impls::rabenseifner_scatter {

class rabenseifner_scatter : public dsop {
  using dsop::dsop;

 public:
  void compute(const std::vector<vector>& a_in, const std::vector<vector>& b_in, matrix& result) override;
};

struct SubVec {
  int n;         // number of elements in the double array
  int start_idx; // start and end index, where subvec starts and ends
  int end_idx;
  int is_A; // 0: vector B, !=0: vector A
  int owner_rank;
  vector* vecPtr;
};

void addReceivedVectors(
    std::vector<SubVec>* buffer_A, std::vector<SubVec>* buffer_B, double* receiveBuffer, int n_recv_elements);
void setSendVector(std::vector<SubVec>* buffer_A, std::vector<SubVec>* buffer_B, double* sendBuffer,
    int* n_send_elements, int lower_index_A, int lower_index_B, int upper_index_A, int upper_index_B);

void submatrixToBuffer(matrix& M, double* sendbuffer, int start_row, int end_row, int start_col, int end_col);
void copyBufferToSubmatrix(matrix& M, double* sendbuffer, int start_row, int end_row, int start_col, int end_col);

} // namespace impls::rabenseifner_scatter
