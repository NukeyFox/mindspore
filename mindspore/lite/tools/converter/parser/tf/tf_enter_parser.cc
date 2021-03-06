/**
 * Copyright 2021 Huawei Technologies Co., Ltd
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

#include <string>
#include <map>
#include <vector>
#include "tools/converter/parser/tf/tf_enter_parser.h"
#include "tools/converter/parser/tf/tf_node_parser_registry.h"
#include "tools/converter/ops/enter.h"

namespace mindspore {
namespace lite {
STATUS TFEnterParser::Parse(const tensorflow::NodeDef &tf_op,
                            const std::map<string, const tensorflow::NodeDef *> &tf_node_map, PrimitiveC **primitiveC,
                            std::vector<std::string> *inputs, int *output_size) {
  MS_LOG(INFO) << "TF EnterParser";
  if (primitiveC == nullptr || output_size == nullptr) {
    MS_LOG(ERROR) << "primitiveC is nullptr";
    return RET_NULL_PTR;
  }

  *primitiveC = new (std::nothrow) Enter();
  if (*primitiveC == nullptr) {
    MS_LOG(ERROR) << "primitiveC is nullptr";
    return RET_ERROR;
  }

  *output_size = tf_op.input_size();
  for (int i = 0; i < tf_op.input_size(); i++) {
    inputs->emplace_back(tf_op.input(i));
  }

  return RET_OK;
}
TFNodeRegistrar g_tfEnterParser("Enter", new TFEnterParser());
}  // namespace lite
}  // namespace mindspore
