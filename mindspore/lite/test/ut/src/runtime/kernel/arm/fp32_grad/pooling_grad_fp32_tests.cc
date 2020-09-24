/**
 * Copyright 2020 Huawei Technologies Co., Ltd
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <iostream>
#include <memory>
#include "mindspore/lite/include/context.h"
#include "src/common/log_adapter.h"
#include "common/common_test.h"
#include "mindspore/lite/src/kernel_registry.h"
#include "src/common/utils.h"
#include "src/common/file_utils.h"
#include "src/common/file_utils_ext.h"
#include "src/runtime/kernel/arm/fp32_grad/pooling_grad.h"
#include "nnacl/fp32_grad/pooling_grad.h"

namespace mindspore {
class TestPoolingGradFp32 : public mindspore::CommonTest {
 public:
  TestPoolingGradFp32() {}
};

void InitPoolingParamFP32(PoolingParameter *pooling_param) {
  pooling_param->input_batch_ = 1;
  pooling_param->input_h_ = 28;
  pooling_param->input_w_ = 28;
  pooling_param->input_channel_ = 3;

  pooling_param->output_batch_ = 1;
  pooling_param->output_h_ = 28;
  pooling_param->output_w_ = 28;
  pooling_param->output_channel_ = 32;

  pooling_param->window_h_ = 3;
  pooling_param->window_w_ = 3;

  pooling_param->stride_h_ = 1;
  pooling_param->stride_w_ = 1;

  pooling_param->pad_u_ = 1;
  pooling_param->pad_d_ = 1;
  pooling_param->pad_l_ = 1;
  pooling_param->pad_r_ = 1;
  pooling_param->thread_num_ = 1;
  pooling_param->global_ = false;
}

TEST_F(TestPoolingGradFp32, AvgPoolingGradFp32) {
  // prepare stage
  auto pooling_param = static_cast<PoolingParameter *>(malloc(sizeof(PoolingParameter)));
  InitPoolingParamFP32(pooling_param);
  pooling_param->output_channel_ = 3;
  pooling_param->pool_mode_ = PoolMode_AvgPool;

  // runtime part
  printf("Calculating runtime cost...\n");
  uint64_t time_avg = 0;
  size_t output_data_size =
    pooling_param->output_batch_ * pooling_param->output_channel_ * pooling_param->output_h_ * pooling_param->output_w_;

  size_t input_size;
  std::string input_path = "./test_data/pooling/avgpoolgradfp32_1_dy_1_28_28_3.bin";
  auto input_data = reinterpret_cast<float *>(mindspore::lite::ReadFile(input_path.c_str(), &input_size));

  auto output_data = new float[output_data_size];
  // warm up loop
  for (int i = 0; i < 3; i++) {
    AvgPoolingGrad(input_data, output_data, pooling_param);
  }

  int loop_count = 100;
  auto time_start = mindspore::lite::GetTimeUs();
  for (int i = 0; i < loop_count; i++) {
    AvgPoolingGrad(input_data, output_data, pooling_param);
  }
  auto time_end = mindspore::lite::GetTimeUs();
  auto cost = time_end - time_start;
  time_avg = cost / loop_count;
  printf("single thread running time : %f ms\n", time_avg / 1000.0f);

  printf("==================output data=================\n");
  for (int i = 0; i < 20; i++) {
    std::cout << output_data[i] << " ,";
  }
  std::cout << std::endl;
  std::string output_path = "./test_data/pooling/avgpoolgradfp32_1_dx_1_28_28_3.bin";
  auto res = lite::CompareOutput(output_data, output_path);
  EXPECT_EQ(res, 0);

  delete[] input_data;
  delete[] output_data;
  free(pooling_param);
  MS_LOG(INFO) << "TestAvgPoolingGradFp32 passed";
}

TEST_F(TestPoolingGradFp32, AvgPoolingKernelGradFp32) {
  // prepare stage
  auto pooling_param = static_cast<PoolingParameter *>(malloc(sizeof(PoolingParameter)));
  InitPoolingParamFP32(pooling_param);
  pooling_param->output_channel_ = 3;
  pooling_param->pool_mode_ = PoolMode_AvgPool;

  // runtime part
  printf("Calculating runtime cost...\n");
  // uint64_t time_avg = 0;
  size_t output_data_size =
    pooling_param->output_batch_ * pooling_param->output_channel_ * pooling_param->output_h_ * pooling_param->output_w_;

  size_t input_size;
  std::string input_path = "./test_data/pooling/avgpoolgradfp32_1_dy_1_28_28_3.bin";
  auto input_data = reinterpret_cast<float *>(mindspore::lite::ReadFile(input_path.c_str(), &input_size));
  std::vector<int> dim_dy({1, 28, 28, 3});
  lite::Tensor dy_tensor(TypeId::kNumberTypeFloat32, dim_dy);
  dy_tensor.SetData(input_data);

  std::string input1_path = "./test_data/pooling/avgpoolgradfp32_1_x_1_28_28_3.bin";
  auto input1_data = reinterpret_cast<float *>(mindspore::lite::ReadFile(input1_path.c_str(), &input_size));
  std::vector<int> dim_x({1, 28, 28, 3});
  lite::Tensor x_tensor(TypeId::kNumberTypeFloat32, dim_x);
  x_tensor.SetData(input1_data);

  std::vector<lite::Tensor *> inputs = {&dy_tensor, &x_tensor};

  auto output_data = new float[output_data_size];
  std::vector<int> dim_dx({1, 28, 28, 3});
  lite::Tensor dx_tensor(TypeId::kNumberTypeFloat32, dim_dx);
  dx_tensor.SetData(output_data);
  std::vector<lite::Tensor *> outputs = {&dx_tensor};

  kernel::KernelKey desc = {kernel::kCPU, TypeId::kNumberTypeFloat32, schema::PrimitiveType_PoolingGrad};

  auto creator = lite::KernelRegistry::GetInstance()->GetCreator(desc);
  auto kernel_obj = creator(inputs, outputs, reinterpret_cast<OpParameter *>(pooling_param), NULL, desc, nullptr);

  kernel_obj->Run();

  printf("==================output data=================\n");
  for (int i = 0; i < 20; i++) {
    std::cout << output_data[i] << " ,";
  }
  std::cout << std::endl;
  std::string output_path = "./test_data/pooling/avgpoolgradfp32_1_dx_1_28_28_3.bin";
  auto res = lite::CompareOutput(output_data, output_path);
  EXPECT_EQ(res, 0);

  delete[] input_data;
  delete[] input1_data;
  delete[] output_data;
  dx_tensor.SetData(nullptr);
  x_tensor.SetData(nullptr);
  dy_tensor.SetData(nullptr);
  // delete pooling_param;
  delete kernel_obj;
  MS_LOG(INFO) << "TestAvgPoolingGradFp32 passed";
}

TEST_F(TestPoolingGradFp32, AvgPoolingBatchGradFp32) {
  // prepare stage
  auto pooling_param = static_cast<PoolingParameter *>(malloc(sizeof(PoolingParameter)));
  InitPoolingParamFP32(pooling_param);

  pooling_param->output_channel_ = 3;
  pooling_param->input_batch_ = 3;
  pooling_param->output_batch_ = 3;
  pooling_param->pool_mode_ = PoolMode_AvgPool;

  // runtime part
  printf("Calculating runtime cost...\n");
  // uint64_t time_avg = 0;
  size_t input_size;
  std::string input_path = "./test_data/pooling/avgpoolgradfp32_1_dy_3_28_28_3.bin";
  auto input_data = reinterpret_cast<float *>(mindspore::lite::ReadFile(input_path.c_str(), &input_size));
  std::vector<int> dim_dy({3, 28, 28, 3});
  lite::Tensor dy_tensor(TypeId::kNumberTypeFloat32, dim_dy);
  dy_tensor.SetData(input_data);

  std::string input1_path = "./test_data/pooling/avgpoolgradfp32_1_x_3_28_28_3.bin";
  auto input1_data = reinterpret_cast<float *>(mindspore::lite::ReadFile(input1_path.c_str(), &input_size));
  std::vector<int> dim_x({3, 28, 28, 3});
  lite::Tensor x_tensor(TypeId::kNumberTypeFloat32, dim_x);
  x_tensor.SetData(input1_data);

  std::vector<lite::Tensor *> inputs = {&dy_tensor, &x_tensor};

  std::vector<int> dim_dx({3, 28, 28, 3});
  lite::Tensor dx_tensor(TypeId::kNumberTypeFloat32, dim_dx);
  ASSERT_EQ(dx_tensor.MallocData(), 0);
  auto output_data = reinterpret_cast<float *>(dx_tensor.MutableData());
  std::vector<lite::Tensor *> outputs = {&dx_tensor};

  kernel::KernelKey desc = {kernel::kCPU, TypeId::kNumberTypeFloat32, schema::PrimitiveType_PoolingGrad};

  auto creator = lite::KernelRegistry::GetInstance()->GetCreator(desc);
  auto kernel_obj = creator(inputs, outputs, reinterpret_cast<OpParameter *>(pooling_param), NULL, desc, nullptr);

  kernel_obj->Run();

  printf("==================output data=================\n");
  for (int i = 0; i < 20; i++) {
    std::cout << output_data[i] << " ,";
  }
  std::cout << std::endl;
  std::string output_path = "./test_data/pooling/avgpoolgradfp32_1_dx_3_28_28_3.bin";
  auto res = lite::CompareOutput(output_data, output_path);
  EXPECT_EQ(res, 0);

  delete[] input_data;
  delete[] input1_data;
  x_tensor.SetData(nullptr);
  dy_tensor.SetData(nullptr);
  // delete pooling_param;
  delete kernel_obj;
  MS_LOG(INFO) << "TestAvgPoolingGradBatchFp32 passed";
}

TEST_F(TestPoolingGradFp32, AvgPoolGradStride2Fp32) {
  // prepare stage
  // input size will be equal to the original size of x, output size will be the output size as in forward
  auto pool = static_cast<PoolingParameter *>(malloc(sizeof(PoolingParameter)));
  InitPoolingParamFP32(pool);
  pool->output_channel_ = 3;
  pool->pool_mode_ = PoolMode_AvgPool;
  pool->input_batch_ = 3;
  pool->output_batch_ = 3;
  pool->output_h_ = 14;
  pool->output_w_ = 14;
  pool->stride_h_ = 2;
  pool->stride_w_ = 2;

  size_t input_size;

  auto x_data = reinterpret_cast<float *>(
    mindspore::lite::ReadFile("./test_data/pooling/avgpoolgradfp32_s2_x_3_28_28_3.bin", &input_size));
  std::vector<int> dim_x({pool->output_batch_, pool->input_h_, pool->input_w_, pool->input_channel_});
  lite::Tensor x_tensor(TypeId::kNumberTypeFloat32, dim_x);
  x_tensor.SetData(x_data);

  auto yt_data = reinterpret_cast<float *>(
    mindspore::lite::ReadFile("./test_data/pooling/avgpoolgradfp32_s2_dy_3_28_28_3.bin", &input_size));
  std::vector<int> dim_y({pool->output_batch_, pool->output_h_, pool->output_w_, pool->output_channel_});
  lite::Tensor yt_tensor(TypeId::kNumberTypeFloat32, dim_y);
  yt_tensor.SetData(yt_data);
  lite::Tensor out_tensor(TypeId::kNumberTypeFloat32, dim_x);
  ASSERT_EQ(out_tensor.MallocData(), 0);
  float *out_data = static_cast<float *>(out_tensor.MutableData());
  std::vector<lite::Tensor *> inputs = {&yt_tensor, &x_tensor};
  std::vector<lite::Tensor *> outputs = {&out_tensor};
  // ----------------------------------------
  kernel::KernelKey pool_desc = {kernel::kCPU, TypeId::kNumberTypeFloat32, schema::PrimitiveType_PoolingGrad};
  auto pool_creator = lite::KernelRegistry::GetInstance()->GetCreator(pool_desc);
  auto kernel = pool_creator(inputs, outputs, reinterpret_cast<OpParameter *>(pool), NULL, pool_desc, nullptr);

  kernel->Init();

  auto time_start = mindspore::lite::GetTimeUs();
  kernel->Run();
  auto time_end = mindspore::lite::GetTimeUs();
  printf("single thread running time : %llu ms\n", time_end - time_start);

  std::string output_path = "./test_data/pooling/avgpoolgradfp32_s2_dx_3_28_28_3.bin";
  auto res = lite::CompareRelativeOutput(out_data, output_path);
  EXPECT_EQ(res, 0);

  delete[] x_data;
  delete[] yt_data;
  // delete[] out_data;
  // delete conv_param;
  x_tensor.SetData(nullptr);
  yt_tensor.SetData(nullptr);
  delete kernel;
  MS_LOG(INFO) << "AvgPoolGradStride2Fp32 Filter Grad passed";
}

TEST_F(TestPoolingGradFp32, AvgPoolGradStride3Fp32) {
  // prepare stage
  // input size will be equal to the original size of x, output size will be the output size as in forward
  auto pool = static_cast<PoolingParameter *>(malloc(sizeof(PoolingParameter)));
  InitPoolingParamFP32(pool);
  pool->output_channel_ = 3;
  pool->pool_mode_ = PoolMode_AvgPool;
  pool->input_batch_ = 3;
  pool->output_batch_ = 3;
  pool->output_h_ = 10;
  pool->output_w_ = 10;
  pool->stride_h_ = 3;
  pool->stride_w_ = 3;

  size_t input_size;

  auto x_data = reinterpret_cast<float *>(
    mindspore::lite::ReadFile("./test_data/pooling/avgpoolgradfp32_s3_x_3_28_28_3.bin", &input_size));
  std::vector<int> dim_x({pool->output_batch_, pool->input_h_, pool->input_w_, pool->input_channel_});
  lite::Tensor x_tensor(TypeId::kNumberTypeFloat32, dim_x);
  x_tensor.SetData(x_data);

  auto yt_data = reinterpret_cast<float *>(
    mindspore::lite::ReadFile("./test_data/pooling/avgpoolgradfp32_s3_dy_3_28_28_3.bin", &input_size));
  std::vector<int> dim_y({pool->output_batch_, pool->output_h_, pool->output_w_, pool->output_channel_});
  lite::Tensor yt_tensor(TypeId::kNumberTypeFloat32, dim_y);
  yt_tensor.SetData(yt_data);

  lite::Tensor out_tensor(TypeId::kNumberTypeFloat32, dim_x);
  ASSERT_EQ(out_tensor.MallocData(), 0);
  auto out_data = static_cast<float *>(out_tensor.MutableData());

  std::vector<lite::Tensor *> inputs = {&yt_tensor, &x_tensor};
  std::vector<lite::Tensor *> outputs = {&out_tensor};
  // ----------------------------------------
  kernel::KernelKey pool_desc = {kernel::kCPU, TypeId::kNumberTypeFloat32, schema::PrimitiveType_PoolingGrad};
  auto pool_creator = lite::KernelRegistry::GetInstance()->GetCreator(pool_desc);
  auto kernel = pool_creator(inputs, outputs, reinterpret_cast<OpParameter *>(pool), NULL, pool_desc, nullptr);

  kernel->Init();

  auto time_start = mindspore::lite::GetTimeUs();
  kernel->Run();
  auto time_end = mindspore::lite::GetTimeUs();
  printf("single thread running time : %llu ms\n", time_end - time_start);

  std::string output_path = "./test_data/pooling/avgpoolgradfp32_s3_dx_3_28_28_3.bin";
  auto res = lite::CompareRelativeOutput(out_data, output_path);

  EXPECT_EQ(res, 0);

  delete[] x_data;
  delete[] yt_data;
  // delete[] out_data;
  // delete conv_param;
  x_tensor.SetData(nullptr);
  yt_tensor.SetData(nullptr);
  delete kernel;
  MS_LOG(INFO) << "AvgPoolGradStride3Fp32 Filter Grad passed";
}

TEST_F(TestPoolingGradFp32, MaxPoolingGradFp32) {
  // prepare stage
  auto pooling_param = static_cast<PoolingParameter *>(malloc(sizeof(PoolingParameter)));
  InitPoolingParamFP32(pooling_param);
  pooling_param->output_channel_ = 3;
  pooling_param->pool_mode_ = PoolMode_MaxPool;
  // runtime part
  printf("Calculating runtime cost...\n");
  uint64_t time_avg = 0;
  size_t output_data_size =
    pooling_param->output_batch_ * pooling_param->output_channel_ * pooling_param->output_h_ * pooling_param->output_w_;

  size_t input_size;
  std::string i_path = "./test_data/pooling/maxpoolgradfp32_1_x_1_28_28_3.bin";
  auto in_data = reinterpret_cast<float *>(mindspore::lite::ReadFile(i_path.c_str(), &input_size));

  std::string dy_path = "./test_data/pooling/maxpoolgradfp32_1_dy_1_28_28_3.bin";
  auto dy_data = reinterpret_cast<float *>(mindspore::lite::ReadFile(dy_path.c_str(), &input_size));

  std::string dx_path = "./test_data/pooling/maxpoolgradfp32_1_dx_1_28_28_3.bin";
  auto dx_data = reinterpret_cast<float *>(mindspore::lite::ReadFile(dx_path.c_str(), &input_size));

  auto output_data = new float[output_data_size];
  // warm up loop
  for (int i = 0; i < 3; i++) {
    MaxPoolingGrad(in_data, dx_data, dy_data, output_data, pooling_param);
  }

  int loop_count = 100;
  auto time_start = mindspore::lite::GetTimeUs();
  for (int i = 0; i < loop_count; i++) {
    MaxPoolingGrad(in_data, dx_data, dy_data, output_data, pooling_param);
  }
  auto time_end = mindspore::lite::GetTimeUs();
  auto cost = time_end - time_start;
  time_avg = cost / loop_count;
  printf("single thread running time : %f ms\n", time_avg / 1000.0f);

  printf("==================output data=================\n");
  for (int i = 0; i < 20; i++) {
    std::cout << output_data[i] << " ,";
  }
  std::cout << std::endl;
  std::string output_path = "./test_data/pooling/maxpoolgradfp32_1_xgrad_1_28_28_3.bin";
  auto res = lite::CompareOutput(output_data, output_path);
  EXPECT_EQ(res, 0);

  free(pooling_param);
  delete[] in_data;
  delete[] dy_data;
  delete[] dx_data;
  delete[] output_data;
  MS_LOG(INFO) << "TestMaxPoolingGradFp32 passed";
}

TEST_F(TestPoolingGradFp32, MaxPoolGradBatchFp32) {
  // prepare stage
  // input size will be equal to the original size of x, output size will be the output size as in forward
  auto maxpool = static_cast<PoolingParameter *>(malloc(sizeof(PoolingParameter)));
  InitPoolingParamFP32(maxpool);
  maxpool->output_channel_ = 3;
  maxpool->pool_mode_ = PoolMode_MaxPool;
  maxpool->input_batch_ = 3;
  maxpool->output_batch_ = 3;

  size_t input_size;

  auto x_data = reinterpret_cast<float *>(
    mindspore::lite::ReadFile("./test_data/pooling/maxpoolgradfp32_1_x_3_28_28_3.bin", &input_size));
  std::vector<int> dim_x({3, 28, 28, 3});
  lite::Tensor x_tensor(TypeId::kNumberTypeFloat32, dim_x);
  x_tensor.SetData(x_data);

  auto y_data = reinterpret_cast<float *>(
    mindspore::lite::ReadFile("./test_data/pooling/maxpoolgradfp32_1_dx_3_28_28_3.bin", &input_size));
  std::vector<int> dim_y({3, 28, 28, 3});
  lite::Tensor y_tensor(TypeId::kNumberTypeFloat32, dim_y);
  y_tensor.SetData(y_data);

  auto yt_data = reinterpret_cast<float *>(
    mindspore::lite::ReadFile("./test_data/pooling/maxpoolgradfp32_1_dy_3_28_28_3.bin", &input_size));
  lite::Tensor yt_tensor(TypeId::kNumberTypeFloat32, dim_y);
  yt_tensor.SetData(yt_data);

  lite::Tensor out_tensor(TypeId::kNumberTypeFloat32, dim_x);
  ASSERT_EQ(out_tensor.MallocData(), 0);
  auto out_data = static_cast<float *>(out_tensor.MutableData());
  std::vector<lite::Tensor *> maxpool_inputs = {&x_tensor, &y_tensor, &yt_tensor};
  std::vector<lite::Tensor *> maxpool_outputs = {&out_tensor};
  // ----------------------------------------
  kernel::KernelKey maxpool_desc = {kernel::kCPU, TypeId::kNumberTypeFloat32, schema::PrimitiveType_PoolingGrad};
  auto maxpool_creator = lite::KernelRegistry::GetInstance()->GetCreator(maxpool_desc);
  auto kernel = maxpool_creator(maxpool_inputs, maxpool_outputs, reinterpret_cast<OpParameter *>(maxpool), NULL,
                                maxpool_desc, nullptr);

  kernel->Init();

  auto time_start = mindspore::lite::GetTimeUs();
  kernel->Run();
  auto time_end = mindspore::lite::GetTimeUs();
  printf("single thread running time : %llu ms\n", time_end - time_start);

  std::string output_path = "./test_data/pooling/maxpoolgradfp32_1_xgrad_3_28_28_3.bin";
  auto res = lite::CompareRelativeOutput(out_data, output_path);

  EXPECT_EQ(res, 0);

  delete[] x_data;
  delete[] y_data;
  delete[] yt_data;
  // delete[] out_data;
  // delete conv_param;
  x_tensor.SetData(nullptr);
  y_tensor.SetData(nullptr);
  yt_tensor.SetData(nullptr);
  delete kernel;
  MS_LOG(INFO) << "MaxPoolGradBatchFp32 Filter Grad passed";
}

TEST_F(TestPoolingGradFp32, MaxPoolGradStride2Fp32) {
  // prepare stage
  // input size will be equal to the original size of x, output size will be the output size as in forward
  auto maxpool = static_cast<PoolingParameter *>(malloc(sizeof(PoolingParameter)));
  InitPoolingParamFP32(maxpool);
  maxpool->output_channel_ = 3;
  maxpool->input_channel_ = 3;
  maxpool->pool_mode_ = PoolMode_MaxPool;
  maxpool->input_batch_ = 3;
  maxpool->output_batch_ = 3;
  maxpool->output_h_ = 14;
  maxpool->output_w_ = 14;
  maxpool->stride_h_ = 2;
  maxpool->stride_w_ = 2;

  size_t input_size;

  auto x_data = reinterpret_cast<float *>(
    mindspore::lite::ReadFile("./test_data/pooling/maxpoolgradfp32_s2_x_3_28_28_3.bin", &input_size));
  std::vector<int> dim_x({maxpool->output_batch_, maxpool->input_h_, maxpool->input_w_, maxpool->input_channel_});
  lite::Tensor x_tensor(TypeId::kNumberTypeFloat32, dim_x);
  x_tensor.SetData(x_data);

  auto y_data = reinterpret_cast<float *>(
    mindspore::lite::ReadFile("./test_data/pooling/maxpoolgradfp32_s2_dx_3_28_28_3.bin", &input_size));
  std::vector<int> dim_y({maxpool->output_batch_, maxpool->output_h_, maxpool->output_w_, maxpool->output_channel_});
  lite::Tensor y_tensor(TypeId::kNumberTypeFloat32, dim_y);
  y_tensor.SetData(y_data);

  auto yt_data = reinterpret_cast<float *>(
    mindspore::lite::ReadFile("./test_data/pooling/maxpoolgradfp32_s2_dy_3_28_28_3.bin", &input_size));
  lite::Tensor yt_tensor(TypeId::kNumberTypeFloat32, dim_y);
  yt_tensor.SetData(yt_data);

  lite::Tensor out_tensor(TypeId::kNumberTypeFloat32, dim_x);
  ASSERT_EQ(out_tensor.MallocData(), 0);
  auto out_data = static_cast<float *>(out_tensor.MutableData());

  std::vector<lite::Tensor *> maxpool_inputs = {&x_tensor, &y_tensor, &yt_tensor};
  std::vector<lite::Tensor *> maxpool_outputs = {&out_tensor};
  // ----------------------------------------
  kernel::KernelKey maxpool_desc = {kernel::kCPU, TypeId::kNumberTypeFloat32, schema::PrimitiveType_PoolingGrad};
  auto maxpool_creator = lite::KernelRegistry::GetInstance()->GetCreator(maxpool_desc);
  auto kernel = maxpool_creator(maxpool_inputs, maxpool_outputs, reinterpret_cast<OpParameter *>(maxpool), NULL,
                                maxpool_desc, nullptr);

  kernel->Init();

  auto time_start = mindspore::lite::GetTimeUs();
  kernel->Run();
  auto time_end = mindspore::lite::GetTimeUs();
  printf("single thread running time : %llu ms\n", time_end - time_start);

  std::string output_path = "./test_data/pooling/maxpoolgradfp32_s2_xgrad_3_28_28_3.bin";
  auto res = lite::CompareRelativeOutput(out_data, output_path);

  EXPECT_EQ(res, 0);

  delete[] x_data;
  delete[] y_data;
  delete[] yt_data;
  // delete[] out_data;
  // delete conv_param;
  x_tensor.SetData(nullptr);
  y_tensor.SetData(nullptr);
  yt_tensor.SetData(nullptr);
  delete kernel;
  MS_LOG(INFO) << "MaxPoolGradStride2Fp32 Filter Grad passed";
}

TEST_F(TestPoolingGradFp32, MaxPoolGradStride3Fp32) {
  // prepare stage
  // input size will be equal to the original size of x, output size will be the output size as in forward
  auto maxpool = static_cast<PoolingParameter *>(malloc(sizeof(PoolingParameter)));
  InitPoolingParamFP32(maxpool);
  maxpool->output_channel_ = 3;
  maxpool->input_channel_ = 3;
  maxpool->pool_mode_ = PoolMode_MaxPool;
  maxpool->input_batch_ = 3;
  maxpool->output_batch_ = 3;
  maxpool->output_h_ = 10;
  maxpool->output_w_ = 10;
  maxpool->stride_h_ = 3;
  maxpool->stride_w_ = 3;

  size_t input_size;

  auto x_data = reinterpret_cast<float *>(
    mindspore::lite::ReadFile("./test_data/pooling/maxpoolgradfp32_s3_x_3_28_28_3.bin", &input_size));
  std::vector<int> dim_x({maxpool->output_batch_, maxpool->input_h_, maxpool->input_w_, maxpool->input_channel_});
  lite::Tensor x_tensor(TypeId::kNumberTypeFloat32, dim_x);
  x_tensor.SetData(x_data);

  auto y_data = reinterpret_cast<float *>(
    mindspore::lite::ReadFile("./test_data/pooling/maxpoolgradfp32_s3_dx_3_28_28_3.bin", &input_size));
  std::vector<int> dim_y({maxpool->output_batch_, maxpool->output_h_, maxpool->output_w_, maxpool->output_channel_});
  lite::Tensor y_tensor(TypeId::kNumberTypeFloat32, dim_y);
  y_tensor.SetData(y_data);

  auto yt_data = reinterpret_cast<float *>(
    mindspore::lite::ReadFile("./test_data/pooling/maxpoolgradfp32_s3_dy_3_28_28_3.bin", &input_size));
  lite::Tensor yt_tensor(TypeId::kNumberTypeFloat32, dim_y);
  yt_tensor.SetData(yt_data);

  lite::Tensor out_tensor(TypeId::kNumberTypeFloat32, dim_x);
  ASSERT_EQ(out_tensor.MallocData(), 0);
  auto out_data = static_cast<float *>(out_tensor.MutableData());

  std::vector<lite::Tensor *> maxpool_inputs = {&x_tensor, &y_tensor, &yt_tensor};
  std::vector<lite::Tensor *> maxpool_outputs = {&out_tensor};
  // ----------------------------------------
  kernel::KernelKey maxpool_desc = {kernel::kCPU, TypeId::kNumberTypeFloat32, schema::PrimitiveType_PoolingGrad};
  auto maxpool_creator = lite::KernelRegistry::GetInstance()->GetCreator(maxpool_desc);
  auto kernel = maxpool_creator(maxpool_inputs, maxpool_outputs, reinterpret_cast<OpParameter *>(maxpool), NULL,
                                maxpool_desc, nullptr);

  kernel->Init();

  auto time_start = mindspore::lite::GetTimeUs();
  kernel->Run();
  auto time_end = mindspore::lite::GetTimeUs();
  printf("single thread running time : %llu ms\n", time_end - time_start);

  std::string output_path = "./test_data/pooling/maxpoolgradfp32_s3_xgrad_3_28_28_3.bin";
  auto res = lite::CompareRelativeOutput(out_data, output_path);

  EXPECT_EQ(res, 0);

  delete[] x_data;
  delete[] y_data;
  delete[] yt_data;
  // delete[] out_data;
  // delete conv_param;
  x_tensor.SetData(nullptr);
  y_tensor.SetData(nullptr);
  yt_tensor.SetData(nullptr);
  delete kernel;
  MS_LOG(INFO) << "MaxPoolGradStride3Fp32 Filter Grad passed";
}

}  // namespace mindspore
