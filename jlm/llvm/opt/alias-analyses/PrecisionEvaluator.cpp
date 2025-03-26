/*
 * Copyright 2025 Håvard Krogstie <krogstie.havard@gmail.com>
 * See COPYING for terms of redistribution.
 */

#include <jlm/llvm/opt/alias-analyses/PrecisionEvaluator.hpp>
#include <jlm/rvsdg/RvsdgModule.hpp>

namespace jlm::llvm::aa
{

class AAClientStatistics final : public util::Statistics
{
  // This statistic places additional information in a separate file. This is the path of the file.
  static constexpr auto PrecisionDumpFile_ = "DumpFile";
  static constexpr auto PrecisionEvaluationTimer_ = "PrecisionEvaluationTimer";

public:
  ~AAClientStatistics() override = default;

  explicit AAClientStatistics(const util::filepath & sourceFile)
      : Statistics(Id::AliasAnalysisClientPrecision, sourceFile)
  {}

  void
  StartEvaluatingPrecision()
  {
    AddTimer(PrecisionEvaluationTimer_).start();
  }

  void
  StopEvaluatingPrecision(const util::filepath & path)
  {
    GetTimer(PrecisionEvaluationTimer_).stop();
    AddMeasurement(PrecisionDumpFile_, path.to_str());
  }

  static std::unique_ptr<AAClientStatistics>
  Create(const util::filepath & sourceFile)
  {
    return std::make_unique<AAClientStatistics>(sourceFile);
  }
};

void
PrecisionEvaluator::EvaluateAliasAnalysisClient(
    const rvsdg::RvsdgModule & rvsdgModule,
    const PointsToGraph & pointsToGraph,
    util::StatisticsCollector & statisticsCollector)
{
  auto statistics = AAClientStatistics::Create(rvsdgModule.SourceFilePath().value());

  // If the precision evaluation is not demanded, skip doing it
  if (!statisticsCollector.IsDemanded(*statistics))
    return;

  statistics->StartEvaluatingPrecision();

  auto file = statisticsCollector.CreateOutputFile();

  statistics->StopEvaluatingPrecision(file.path());
  statisticsCollector.CollectDemandedStatistics(std::move(statistics));
}

}
