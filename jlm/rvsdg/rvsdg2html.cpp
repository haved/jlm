// Created by Håvard Krogstie on 2023-09-18

#include <jlm/rvsdg/rvsdg2html.hpp>
#include <jlm/rvsdg/node.hpp>
#include <jlm/rvsdg/structural-node.hpp>
#include <jlm/rvsdg/simple-node.hpp>
#include <jlm/util/strfmt.hpp>

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

class Rvsdg2Html {
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
    TagWithId(const char* node, std::string id)
    {
      ss_ << "<" << node << " id=\"" << id << "\"></" << node << ">" << std::endl;
    }

    inline void
    EdgeFromTo(jlm::rvsdg::output * from, jlm::rvsdg::input * to)
    {
      ss_ << "<rvsdg-edge from=\"" << id(from) << "\" to=\"" << id(to) << "\"></rvsdg-edge>" << std::endl;
    }

    void NodeToHtml(const jlm::rvsdg::node& node);
    void RegionToHtml(const jlm::rvsdg::region& region, bool toplevel = false);

public:
    Rvsdg2Html() : ss_() {}

    std::string
    ToHtml(const jlm::rvsdg::region * region);
};

void
Rvsdg2Html::NodeToHtml(const jlm::rvsdg::node &node) {
  auto structural_node = dynamic_cast<const jlm::rvsdg::structural_node*>(&node);

  ss_ << "<rvsdg-node";
  //if (structural_node)
  //  ss_ << " type=\"" << structural_node->operation().debug_string() << '"';
  ss_ << ">" << std::endl;

  TagStart("rvsdg-inputs");
  for (size_t n = 0; n < node.ninputs(); n++)
    TagWithId("rvsdg-input", id(node.input(n)));
  TagEnd("rvsdg-inputs");

  TagStart("div");
  ss_ << node.operation().debug_string();
  TagEnd("div");

  if (structural_node) {
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

  if (!toplevel) {
    TagStart("rvsdg-region-handle");
    TagEnd("rvsdg-region-handle");
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