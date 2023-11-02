// Created by Håvard Krogstie on 2023-09-18

#include <jlm/llvm/backend/rvsdg2html/rvsdg2html.hpp>

#include <jlm/rvsdg/node.hpp>
#include <jlm/rvsdg/structural-node.hpp>
#include "jlm/rvsdg/gamma.hpp"
#include "jlm/rvsdg/theta.hpp"

#include <jlm/util/strfmt.hpp>
#include "jlm/llvm/ir/operators/alloca.hpp"
#include "jlm/llvm/ir/operators/call.hpp"
#include "jlm/llvm/ir/operators/delta.hpp"
#include "jlm/llvm/ir/operators/GetElementPtr.hpp"
#include "jlm/llvm/ir/operators/load.hpp"
#include "jlm/llvm/ir/operators/store.hpp"

namespace jlm::rvsdg {

static const char *html_prologue =
  "<!doctype html>\n"
  "<html lang=\"en\">\n"
  "<head>\n"
  "<meta charset=\"utf-8\">\n"
  "<meta http-equiv=\"x-ua-compatible\" content=\"ie=edge\">\n"
  "<title>RVSDG-to-html</title>\n"
  "<meta name=\"description\" content=\"\">\n"
  "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
  "<link rel=\"stylesheet\" href=\"style.css\">\n"
  "</head>\n"
  "<body>\n"
  "<main>\n";

static const char *html_epilogue =
  "</main>\n"
  "<script src=\"script.js\"></script>\n"
  "</body>\n"
  "</html>";

class Rvsdg2Html
{
  std::stringstream ss_;

  static inline std::string
  id(const jlm::rvsdg::output * port)
  {
    return jlm::util::strfmt("o", (intptr_t)port);
  }

  static inline std::string
  id(const jlm::rvsdg::input * port)
  {
    return jlm::util::strfmt("i", (intptr_t)port);
  }

  void
  PrintEscapedHtml(const std::string& string)
  {
    for (char c: string) {
      if (c == '<')
        ss_ << "&lt;";
      else if (c == '>')
        ss_ << "&gt;";
      else
        ss_ << c;
    }
  }

  inline void
  TagStart(const char* node)
  {
    ss_ << "<" << node << ">" << std::endl;
  }

  inline void
  TagEnd(const char* node)
  {
    ss_ << "</" << node << ">" << std::endl;
  }

  inline void
  TagWithId(const char* node, const std::string& id)
  {
    ss_ << "<" << node << " id=\"" << id << "\"></" << node << ">" << std::endl;
  }

  const char* NodeToKindString(const jlm::rvsdg::node& node);
  void NodeToDescriptionTag(const jlm::rvsdg::node &node);
  const char* EdgeTypeToString(const jlm::rvsdg::type& type);
  void EdgeFromTo(jlm::rvsdg::output * from, jlm::rvsdg::input * to);
  void NodeToHtml(const jlm::rvsdg::node& node);
  void RegionToHtml(const jlm::rvsdg::region& region, bool toplevel = false);

public:
  Rvsdg2Html() : ss_() {}

  std::string
  ToHtml(const jlm::rvsdg::region * region);
};

/**
 * The script for navigating and rendering the RVSDG,
 * may want to handle certain kinds of nodes in special ways.
 * To avoid relying on debug_string() being stable,
 * these nodes get static strings defined here.
 */
const char*
Rvsdg2Html::NodeToKindString(const jlm::rvsdg::node &node) {
  // structural node operations
  if (is<jlm::rvsdg::gamma_op>(&node))
    return "gamma";
  if (is<jlm::rvsdg::theta_op>(&node))
    return "theta";

  // structural node operations, LLVM specific
  if (is<jlm::llvm::delta::operation>(&node))
    return "delta";
  if (is<jlm::llvm::lambda::operation>(&node))
    return "lambda";
  if (is<jlm::llvm::phi::operation>(&node))
    return "phi";

  // common operations
  if (is<jlm::llvm::alloca_op>(&node))
    return "alloca";
  if (is<jlm::llvm::CallOperation>(&node))
    return "call";
  if (is<jlm::llvm::GetElementPtrOperation>(&node))
    return "getElementPtr";
  if (is<jlm::llvm::LoadOperation>(&node))
    return "load";
  if (is<jlm::llvm::StoreOperation>(&node))
    return "store";

  // Other operations are not noteworthy enough for getting a kind
  // The full debug_string() will be included as text for the user
  return "";
}

/**
 * Prints an html tag containing a human readable description of the node
 */
void
Rvsdg2Html::NodeToDescriptionTag(const jlm::rvsdg::node &node) {
  TagStart("rvsdg-node-desc");
  PrintEscapedHtml(node.operation().debug_string());
  TagEnd("rvsdg-node-desc");
}

/**
 * Converts a RVSDG type into a string readable by the rendering script.
 */
const char*
Rvsdg2Html::EdgeTypeToString(const jlm::rvsdg::type &type) {
  if (is<jlm::llvm::MemoryStateType>(type))
    return "memoryStateType";
  if (is<jlm::llvm::iostatetype>(type))
    return "ioStateType";
  if (is<jlm::llvm::loopstatetype>(type))
    return "loopStateType";
  if (is<jlm::llvm::varargtype>(type))
    return "varArgType";
  return "";
}

void
Rvsdg2Html::EdgeFromTo(jlm::rvsdg::output * from, jlm::rvsdg::input * to)
{
  ss_ << "<rvsdg-edge from=\"" << id(from) << "\" to=\"" << id(to)
      << "\" type=\"" << EdgeTypeToString(from->type()) << "\"></rvsdg-edge>" << std::endl;
}

void
Rvsdg2Html::NodeToHtml(const jlm::rvsdg::node &node) {
  ss_ << "<rvsdg-node kind=\"" << NodeToKindString(node) << "\">";

  TagStart("rvsdg-inputs");
  for (size_t n = 0; n < node.ninputs(); n++)
    TagWithId("rvsdg-input", id(node.input(n)));
  TagEnd("rvsdg-inputs");

  NodeToDescriptionTag(node);

  if (auto structural_node = dynamic_cast<const jlm::rvsdg::structural_node*>(&node)) {
    TagStart("rvsdg-regions");
    for (size_t n = 0; n < structural_node->nsubregions(); n++)
      RegionToHtml(*structural_node->subregion(n));
    TagEnd("rvsdg-regions");
  }

  TagStart("rvsdg-outputs");
  for (size_t n = 0; n < node.noutputs(); n++)
    TagWithId("rvsdg-output", id(node.output(n)));
  TagEnd("rvsdg-outputs");

  TagEnd("rvsdg-node");

  // Add all edges that feed into this node
  for (size_t n = 0; n < node.ninputs(); n++) {
    auto input = node.input(n);
    EdgeFromTo(input->origin(), input);
  }
}

void
Rvsdg2Html::RegionToHtml(const jlm::rvsdg::region& region, bool toplevel) {
  ss_ << "<rvsdg-region" << (toplevel ? " fullscreen=\"true\">" : ">") << std::endl;

  TagStart("rvsdg-arguments");
  for (size_t n = 0; n < region.narguments(); n++)
    TagWithId("rvsdg-argument", id(region.argument(n)));
  TagEnd("rvsdg-arguments");

  // Add tags for each node, also adds the edges feeding into the nodes
  for (const jlm::rvsdg::node& node : region)
    NodeToHtml(node);

  TagStart("rvsdg-results");
  for (size_t n = 0; n < region.nresults(); n++) {
    TagWithId("rvsdg-result", id(region.result(n)));
  }
  TagEnd("rvsdg-results");

  // Add all edges that feed into the results of this region
  for (size_t n = 0; n < region.nresults(); n++) {
    auto result = region.result(n);
    EdgeFromTo(result->origin(), result);
  }

  TagEnd("rvsdg-region");
}

std::string
Rvsdg2Html::ToHtml(const jlm::rvsdg::region *region) {
  ss_.clear();
  ss_ << html_prologue;
  RegionToHtml(*region, true);
  ss_ << html_epilogue;
  return ss_.str();
}

std::string
ToHtml(const jlm::rvsdg::region * region) {
  return Rvsdg2Html().ToHtml(region);
}

} // namespace jlm::rvsdg