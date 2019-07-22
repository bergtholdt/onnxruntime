// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "core/graph/onnx_protobuf.h"
#include "core/graph/graph.h"

namespace onnxruntime {

namespace graph_utils {

/** Checks if the operator's type, version, and domain of the given node match the given values. */
bool IsSupportedOptypeVersionAndDomain(const Node& node,
                                       const std::string& op_type,
                                       const std::initializer_list<ONNX_NAMESPACE::OperatorSetVersion>& versions,
                                       const std::string& domain = kOnnxDomainAlias);

/** Checks if the node has the same operator since version as the given one. */
bool MatchesOpSinceVersion(const Node& node, const std::initializer_list<ONNX_NAMESPACE::OperatorSetVersion>& versions);

/** Checks if the node has the same op set domain as the given one. */
bool MatchesOpSetDomain(const Node& node, const std::string& domain);

/** Returns true if the execution provider assigned to current node is present in the compatible providers list
    or if the compatible_providers list is empty. */
bool IsSupportedProvider(const Node& node,
                         const std::unordered_set<std::string>& compatible_providers);

/** Checks if the output at the specified index is input to downstream Nodes. */
bool IsOutputUsed(const Node& node, int index);

/** Returns true if the graph has the given input.*/
bool IsGraphInput(const Graph& graph, const NodeArg* input);

/** returns true if 'name' is an initializer, and is constant and cannot be overridden at runtime. 
@param check_outer_scope If true and the graph is a subgraph, check ancestor graph/s for 'name' if not found in 'graph'.
*/
bool IsConstantInitializer(const Graph& graph, const std::string& name, bool check_outer_scope = true);

/** returns the initializer's TensorProto if 'name' is an initializer, and is constant and 
cannot be overridden at runtime. If the initializer is not found or is not constant a nullptr is returned.
@param check_outer_scope If true and the graph is a subgraph, check ancestor graph/s for 'name' if not found in 'graph'.
*/
const ONNX_NAMESPACE::TensorProto* GetConstantInitializer(const Graph& graph, const std::string& name,
                                                          bool check_outer_scope = true);

/** Add a new initializer to 'graph'. 
Checks that new_initializer does not already exist in 'graph' before adding it.
@returns The NodeArg for the new initializer. 
@remarks No matching graph input is created, so the initializer will be constant. 
*/
NodeArg& AddInitializer(Graph& graph, const ONNX_NAMESPACE::TensorProto& new_initializer);

/** Checks if the given NodeArg is constant, i.e., it appears in the graph's initializers but not in its inputs. */
bool NodeArgIsConstant(const Graph& graph, const NodeArg& node_arg);

/** Checks if the given node has only constant inputs (initializers) and if so returns them in constant_inputs as they
may come from outer scope. */
bool AllNodeInputsAreConstant(const Graph& graph, const Node& node, InitializedTensorSet& constant_inputs);

/** Gets the name of the incoming NodeArg with the specified index for the given node. */
const std::string& GetNodeInputName(const Node& node, int index);

/** Gets the name of the outgoing NodeArg with the specified index for the given node. */
const std::string& GetNodeOutputName(const Node& node, int index);

/** Returns the attribute of a Node with a given name. */
const ONNX_NAMESPACE::AttributeProto* GetNodeAttribute(const Node& node, const std::string& attr_name);

/** Retrieves the values for a repeated attribute of a node and place them to the values vector. */
template <typename T>
bool GetRepeatedNodeAttributeValues(const Node& node,
                                    const std::string& attr_name,
                                    std::vector<T>& values) {
  const auto* attr = graph_utils::GetNodeAttribute(node, attr_name);
  if (attr) {
    values = ONNX_NAMESPACE::RetrieveValues<T>(*attr);
    return true;
  }
  return false;
}

/** Check if it will be possible to remove or fuse a node.
    We support the removal of the Node as long as the following conditions hold:
    - The node should not produce a graph output unless something else will produce that output.
    - Only one of the outputs is used by downstream operators (multiple output edges are allowed).
    - If the Node has a single incoming node, we can remove the Node and connect its incoming node to its 
      outgoing nodes, if doing so does not clash with any values in any relevant subgraphs. 
@param removing_output 
       If true the output from the node will be removed and replaced with its input.
       If false a new Node or initializer is being created that will produce output with the same name. In this case
       it is safe to remove the node even if it provides a graph output.*/
bool CanRemoveNode(const Graph& graph, const Node& node, bool removing_output = true);

/** Removes the given Node from the Graph and keeps Graph consistent by rebuilding needed connections.
See CanRemoveNode for details on when removal is allowed.*/
bool RemoveNodeAndUpdateEdges(Graph& graph, Node& node);

/** Remove a node with a single output, and replace its output with the provided NodeArg for an initializer.*/
bool ReplaceNodeWithInitializer(Graph& graph, Node& node, NodeArg& replacement);

/** Removes all output edges from the given Node of the Graph. 
    This should probably be elevated to the Graph API eventually. */
size_t RemoveNodeOutputEdges(Graph& graph, Node& node);

/** Replace the output of a node.
Replaces the output edges from node using the replacement information, and removes the output edges from 'node'.  
@param replacement The node providing the replacement output.
@param replacement_output_idx The index of the output from 'replacement' to use. 
*/
void ReplaceNodeOutput(Graph& graph, Node& node, Node& replacement, int replacement_output_idx);

/** Replace the NodeArg for an input to a node. 
Use this when replacing the input with an initializer. 
*/
void ReplaceInputNodeArg(Node& target, int target_input_idx, NodeArg& replacement);

/** Finalize the fusion of two nodes.
    If replacement_node is nullptr:
      outputs and edges from second_node are moved to first_node. second_node is deleted.
    If replacement_node is provided: 
      the outputs and edges from second_node are moved to replacement_node. both first_node and second_node are deleted.
*/
void FinalizeNodeFusion(Graph& graph, Node& first_node, Node& second_node, Node* replacement_node = nullptr);

}  // namespace graph_utils

}  // namespace onnxruntime
