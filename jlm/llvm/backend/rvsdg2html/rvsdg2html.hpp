// Created by Håvard Krogstie on 2023-09-18

#ifndef JLM_RVSDG2HTML_HPP
#define JLM_RVSDG2HTML_HPP

#include <string>
#include "jlm/rvsdg/region.hpp"

namespace jlm::rvsdg {

std::string
ToHtml(const jlm::rvsdg::region * region);

} // namespace jlm::rvsdg

#endif //JLM_RVSDG2HTML_HPP
