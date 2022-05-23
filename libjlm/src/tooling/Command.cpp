/*
 * Copyright 2019 Nico Reißmann <nico.reissmann@gmail.com>
 * See COPYING for terms of redistribution.
 */

#include <jlm/tooling/Command.hpp>
#include <jlm/tooling/LlvmPaths.hpp>
#include <jlm/util/strfmt.hpp>

#include <unordered_map>

namespace jlm {

Command::~Command()
= default;

PrintCommandsCommand::~PrintCommandsCommand()
= default;

std::string
PrintCommandsCommand::ToString() const
{
  return "PrintCommands";
}

void
PrintCommandsCommand::Run() const
{
  for (auto & node : CommandGraph::SortNodesTopological(*CommandGraph_)) {
    if (node != &CommandGraph_->GetEntryNode() && node != &CommandGraph_->GetExitNode())
      std::cout << node->GetCommand().ToString() << "\n";
  }
}

std::unique_ptr<CommandGraph>
PrintCommandsCommand::Create(std::unique_ptr<CommandGraph> commandGraph)
{
  std::unique_ptr<CommandGraph> newCommandGraph(new CommandGraph());
  auto & printCommandsNode = PrintCommandsCommand::Create(*newCommandGraph, std::move(commandGraph));
  newCommandGraph->GetEntryNode().AddEdge(printCommandsNode);
  printCommandsNode.AddEdge(newCommandGraph->GetExitNode());
  return newCommandGraph;
}

ClangCommand::~ClangCommand()
= default;

std::string
ClangCommand::ToString() const
{
  std::string inputFiles;
  for (auto & inputFile : InputFiles_)
    inputFiles += inputFile.to_str() + " ";

  std::string libraryPaths;
  for (auto & libraryPath : LibraryPaths_)
    libraryPaths += "-L" + libraryPath + " ";

  std::string libraries;
  for (const auto & library : Libraries_)
    libraries += "-l" + library + " ";

  std::string arguments;
  if (UsePthreads_)
    arguments += "-pthread ";

  return strfmt(
    clangpath.to_str() + " "
    , "-no-pie -O0 "
    , arguments
    , inputFiles
    , "-o ", OutputFile_.to_str(), " "
    , libraryPaths
    , libraries);
}

void
ClangCommand::Run() const
{
  if (system(ToString().c_str()))
    exit(EXIT_FAILURE);
}

LlcCommand::~LlcCommand()
= default;

std::string
LlcCommand::ToString() const
{
  return strfmt(
    llcpath.to_str() + " "
    , "-", ToString(OptimizationLevel_), " "
    , "--relocation-model=", ToString(RelocationModel_), " "
    , "-filetype=obj "
    , "-o ", OutputFile_.to_str(), " "
    , InputFile_.to_str()
  );
}

void
LlcCommand::Run() const
{
  if (system(ToString().c_str()))
    exit(EXIT_FAILURE);
}

std::string
LlcCommand::ToString(const OptimizationLevel & optimizationLevel)
{
  static std::unordered_map<OptimizationLevel, const char*>
    map({
          {OptimizationLevel::O0, "O0"},
          {OptimizationLevel::O1, "O1"},
          {OptimizationLevel::O2, "O2"},
          {OptimizationLevel::O3, "O3"}
        });

  JLM_ASSERT(map.find(optimizationLevel) != map.end());
  return map[optimizationLevel];
}

std::string
LlcCommand::ToString(const RelocationModel & relocationModel)
{
  static std::unordered_map<RelocationModel, const char*>
    map({
          {RelocationModel::Static, "static"},
          {RelocationModel::Pic, "pic"},
        });

  JLM_ASSERT(map.find(relocationModel) != map.end());
  return map[relocationModel];
}

JlmOptCommand::~JlmOptCommand()
= default;

std::string
JlmOptCommand::ToString() const
{
  std::string optimizationArguments;
  for (auto & optimization : Optimizations_)
    optimizationArguments += ToString(optimization) + " ";

  return strfmt(
    "jlm-opt ",
    "--llvm ",
    optimizationArguments,
    "-o ", OutputFile_.to_str(), " ",
    InputFile_.to_str());
}

void
JlmOptCommand::Run() const
{
  if (system(ToString().c_str()))
    exit(EXIT_FAILURE);
}

std::string
JlmOptCommand::ToString(const Optimization & optimization)
{
  static std::unordered_map<Optimization, const char*>
    map({
          {Optimization::AASteensgaardBasic, "--AASteensgaardBasic"},
          {Optimization::CommonNodeElimination, "--cne"},
          {Optimization::DeadNodeElimination, "--dne"},
          {Optimization::FunctionInlining, "--iln"},
          {Optimization::InvariantValueRedirection, "--InvariantValueRedirection"},
          {Optimization::LoopUnrolling, "--url"},
          {Optimization::NodePullIn, "--pll"},
          {Optimization::NodePushOut, "--psh"},
          {Optimization::NodeReduction, "--red"},
          {Optimization::ThetaGammaInversion, "--ivt"}
        });

  JLM_ASSERT(map.find(optimization) != map.end());
  return map[optimization];
}

prscmd::~prscmd()
{}

jlm::filepath
prscmd::ofile() const
{
  return OutputFile_;
}

std::string
prscmd::replace_all(std::string str, const std::string& from, const std::string& to) {
  size_t start_pos = 0;
  while((start_pos = str.find(from, start_pos)) != std::string::npos) {
    str.replace(start_pos, from.length(), to);
    start_pos += to.length();
  }
  return str;
}

std::string
prscmd::ToString() const
{
  auto f = ifile_.base();

  std::string Ipaths;
  for (const auto & Ipath : Ipaths_)
    Ipaths += "-I" + Ipath + " ";

  std::string Dmacros;
  for (const auto & Dmacro : Dmacros_)
    Dmacros += "-D" + Dmacro + " ";

  std::string Wwarnings;
  for (const auto & Wwarning : Wwarnings_)
    Wwarnings += "-W" + Wwarning + " ";

  std::string flags;
  for (const auto & flag : flags_)
    flags += "-f" + flag + " ";

  std::string arguments;
  if (verbose_)
    arguments += "-v ";

  if (rdynamic_)
    arguments += "-rdynamic ";

  if (suppress_)
    arguments += "-w ";

  if (pthread_)
    arguments += "-pthread ";

  if (MD_) {
    arguments += "-MD ";
    arguments += "-MF " + dependencyFile_.to_str() + " ";
    arguments += "-MT " + mT_ + " ";
  }

  std::string languageStandardArgument =
    LanguageStandard_ != LanguageStandard::Unspecified
    ? "-std="+ToString(LanguageStandard_)+" "
    : "";

  if (hls_) {
    return strfmt(
      clangpath.to_str() + " "
      , arguments, " "
      , Wwarnings, " "
      , flags, " "
      , languageStandardArgument
      , replace_all(Dmacros, std::string("\""), std::string("\\\"")), " "
      , Ipaths, " "
      , "-S -emit-llvm -Xclang -disable-O0-optnone "
      , "-o ", OutputFile_.to_str(), " "
      , ifile_.to_str()
    );
  }

  return strfmt(
    clangpath.to_str() + " "
    , arguments, " "
    , Wwarnings, " "
    , flags, " "
    , languageStandardArgument
    , replace_all(Dmacros, std::string("\""), std::string("\\\"")), " "
    , Ipaths, " "
    , "-S -emit-llvm "
    , "-o ", OutputFile_.to_str(), " "
    , ifile_.to_str()
  );
}

void
prscmd::Run() const
{
  if (system(ToString().c_str()))
    exit(EXIT_FAILURE);
}

std::string
prscmd::ToString(const LanguageStandard & languageStandard)
{
  static std::unordered_map<LanguageStandard, const char*>
    map({
          {LanguageStandard::Unspecified, ""},
          {LanguageStandard::Gnu89,       "gnu89"},
          {LanguageStandard::Gnu99,       "gnu99"},
          {LanguageStandard::C89,         "c89"},
          {LanguageStandard::C99,         "c99"},
          {LanguageStandard::C11,         "c11"},
          {LanguageStandard::Cpp98,       "c++98"},
          {LanguageStandard::Cpp03,       "c++03"},
          {LanguageStandard::Cpp11,       "c++11"},
          {LanguageStandard::Cpp14,       "c++14"}
        });

  JLM_ASSERT(map.find(languageStandard) != map.end());
  return map[languageStandard];
}

}
