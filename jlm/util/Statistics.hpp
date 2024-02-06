/*
 * Copyright 2017 Nico Reißmann <nico.reissmann@gmail.com>
 * Copyright 2024 Håvard Krogstie <krogstie.havard@gmail.com>
 * See COPYING for terms of redistribution.
 */

#ifndef JLM_UTIL_STATISTICS_HPP
#define JLM_UTIL_STATISTICS_HPP

#include <jlm/util/file.hpp>
#include <jlm/util/HashSet.hpp>
#include <jlm/util/time.hpp>

#include <cstdint>
#include <list>
#include <memory>
#include <string>
#include <variant>

namespace jlm::util
{

/**
 * \brief Statistics Interface
 */
class Statistics
{
public:
  enum class Id
  {
    FirstEnumValue, // must always be the first enum value, used for iteration

    Aggregation,
    Annotation,
    BasicEncoderEncoding,
    CommonNodeElimination,
    ControlFlowRecovery,
    DataNodeToDelta,
    DeadNodeElimination,
    FunctionInlining,
    InvariantValueRedirection,
    JlmToRvsdgConversion,
    LoopUnrolling,
    MemoryNodeProvisioning,
    PullNodes,
    PushNodes,
    ReduceNodes,
    RvsdgConstruction,
    RvsdgDestruction,
    RvsdgOptimization,
    SteensgaardAnalysis,
    AndersenAnalysis,
    ThetaGammaInversion,

    LastEnumValue // must always be the last enum value, used for iteration
  };

  using Measurement = std::variant<std::string, int64_t, uint64_t, double>;
  // Lists are used instead of vectors to give stable references to members
  using MeasurementList = std::list<std::pair<std::string, Measurement>>;
  using TimerList = std::list<std::pair<std::string, util::timer>>;

  virtual ~Statistics();

  explicit Statistics(const Statistics::Id & statisticsId)
      : StatisticsId_(statisticsId)
  {}

  [[nodiscard]] Statistics::Id
  GetId() const noexcept
  {
    return StatisticsId_;
  }

  [[nodiscard]] virtual std::string
  ToString() const = 0;

  /**
   * Creates a string containing all measurements and timers.
   * Requires all timers to be stopped.
   * @return the created string
   */
  [[nodiscard]] std::string
  Serialize() const;

  /**
   * Checks if a measurement with the given \p name exists.
   * @return true if the measurement exists, false otherwise.
   */
  [[nodiscard]] bool
  HasMeasurement(const std::string & name) const noexcept;

  /**
   * Gets the measurement with the given \p name, it must exist.
   * @return a reference to the measurement.
   */
  [[nodiscard]] const Measurement &
  GetMeasurement(const std::string & name) const;

  /**
   * Gets the value of the measurement with the given \p name.
   * Requires the measurement to exist and have the given type \tparam T.
   * @return the measurement's value.
   */
  template <typename T>
  [[nodiscard]] const T &
  GetMeasurementValue(const std::string & name) const
  {
    const auto & measurement = GetMeasurement(name);
    return std::get<T>(measurement);
  }

  /**
   * Retrieves the full list of measurements
   */
  [[nodiscard]] util::iterator_range<MeasurementList::const_iterator>
  GetMeasurements() const;

  /**
   * Checks if a timer with the given \p name exists.
   * @return true if the timer exists, false otherwise.
   */
  [[nodiscard]] bool
  HasTimer(const std::string & name) const noexcept;

  /**
   * Gets the timer with the given \p name, it must exist.
   * @return the timer.
   */
  [[nodiscard]] const util::timer &
  GetTimer(const std::string & name) const;

  /**
   * Retrieves the full list of timers
   */
  [[nodiscard]] util::iterator_range<TimerList::const_iterator>
  GetTimers() const;

protected:
  /**
   * Adds a measurement, identified by \p name, with the given value.
   * Requires that the measurement doesn't already exist.
   * Measurements are listed in insertion order.
   * @tparam T the type of the measurement, must be one of: std::string, int64_t, uint16_4, double
   */
  template <typename T>
  void
  AddMeasurement(std::string name, T value)
  {
    JLM_ASSERT(!HasMeasurement(name));
    Measurements_.emplace_back(std::make_pair(std::move(name), std::move(value)));
  }


  // Mutable version of @see GetMeasurement
  [[nodiscard]] Measurement &
  GetMeasurement(const std::string & name);

  /**
   * Creates a new timer with the given \p name.
   * Requires that the timer does not already exist.
   * @return a reference to the timer
   */
  util::timer &
  AddTimer(std::string name);

  // Mutable version of @see GetTimer
  [[nodiscard]] util::timer &
  GetTimer(const std::string & name);

private:
  Statistics::Id StatisticsId_;

  // Lists are used instead of vectors to give stable references to members
  MeasurementList Measurements_;
  TimerList Timers_;
};

/**
 * Determines the settings of a StatisticsCollector.
 */
class StatisticsCollectorSettings final
{
public:
  StatisticsCollectorSettings()
      : FilePath_("")
  {}

  explicit StatisticsCollectorSettings(HashSet<Statistics::Id> demandedStatistics)
      : FilePath_(""),
        DemandedStatistics_(std::move(demandedStatistics))
  {}

  StatisticsCollectorSettings(filepath filePath, HashSet<Statistics::Id> demandedStatistics)
      : FilePath_(std::move(filePath)),
        DemandedStatistics_(std::move(demandedStatistics))
  {}

  /** \brief Checks if a statistics is demanded.
   *
   * @param id The Id of the statistics.
   * @return True if a statistics is demanded, otherwise false.
   */
  bool
  IsDemanded(Statistics::Id id) const
  {
    return DemandedStatistics_.Contains(id);
  }

  const filepath &
  GetFilePath() const noexcept
  {
    return FilePath_;
  }

  void
  SetFilePath(filepath filePath)
  {
    FilePath_ = std::move(filePath);
  }

  void
  SetDemandedStatistics(HashSet<Statistics::Id> demandedStatistics)
  {
    DemandedStatistics_ = std::move(demandedStatistics);
  }

  [[nodiscard]] size_t
  NumDemandedStatistics() const noexcept
  {
    return DemandedStatistics_.Size();
  }

  [[nodiscard]] const HashSet<Statistics::Id> &
  GetDemandedStatistics() const noexcept
  {
    return DemandedStatistics_;
  }

  static filepath
  CreateUniqueStatisticsFile(const filepath & directory, const filepath & inputFile)
  {
    return filepath::CreateUniqueFileName(directory, inputFile.base() + "-", "-statistics.log");
  }

private:
  filepath FilePath_;
  HashSet<Statistics::Id> DemandedStatistics_;
};

/**
 * Collects and prints statistics.
 */
class StatisticsCollector final
{
  class StatisticsIterator final
  {
  public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = const Statistics *;
    using difference_type = std::ptrdiff_t;
    using pointer = const Statistics **;
    using reference = const Statistics *&;

  private:
    friend StatisticsCollector;

    explicit StatisticsIterator(const std::vector<std::unique_ptr<Statistics>>::const_iterator & it)
        : it_(it)
    {}

  public:
    [[nodiscard]] const Statistics *
    GetStatistics() const noexcept
    {
      return it_->get();
    }

    const Statistics &
    operator*() const
    {
      JLM_ASSERT(GetStatistics() != nullptr);
      return *GetStatistics();
    }

    const Statistics *
    operator->() const
    {
      return GetStatistics();
    }

    StatisticsIterator &
    operator++()
    {
      ++it_;
      return *this;
    }

    StatisticsIterator
    operator++(int)
    {
      StatisticsIterator tmp = *this;
      ++*this;
      return tmp;
    }

    bool
    operator==(const StatisticsIterator & other) const
    {
      return it_ == other.it_;
    }

    bool
    operator!=(const StatisticsIterator & other) const
    {
      return !operator==(other);
    }

  private:
    std::vector<std::unique_ptr<Statistics>>::const_iterator it_;
  };

public:
  using StatisticsRange = iterator_range<StatisticsIterator>;

  StatisticsCollector()
  {}

  explicit StatisticsCollector(StatisticsCollectorSettings settings)
      : Settings_(std::move(settings))
  {}

  const StatisticsCollectorSettings &
  GetSettings() const noexcept
  {
    return Settings_;
  }

  StatisticsRange
  CollectedStatistics() const noexcept
  {
    return { StatisticsIterator(CollectedStatistics_.begin()),
             StatisticsIterator(CollectedStatistics_.end()) };
  }

  [[nodiscard]] size_t
  NumCollectedStatistics() const noexcept
  {
    return CollectedStatistics_.size();
  }

  /**
   * Checks if the pass statistics is demanded.
   *
   * @param statistics The statistics to check whether it is demanded.
   *
   * @return True if \p statistics is demanded, otherwise false.
   */
  [[nodiscard]] bool
  IsDemanded(const Statistics & statistics) const noexcept
  {
    return GetSettings().IsDemanded(statistics.GetId());
  }

  /**
   * Add \p statistics to collected statistics. A statistics is only added if it is demanded.
   *
   * @param statistics The statistics to collect.
   *
   * @see StatisticsCollectorSettings::IsDemanded()
   */
  void
  CollectDemandedStatistics(std::unique_ptr<Statistics> statistics)
  {
    if (GetSettings().IsDemanded(statistics->GetId()))
      CollectedStatistics_.emplace_back(std::move(statistics));
  }

  /** \brief Print collected statistics to file.
   *
   * @see StatisticsCollectorSettings::GetFilePath()
   */
  void
  PrintStatistics() const;

private:
  StatisticsCollectorSettings Settings_;
  std::vector<std::unique_ptr<Statistics>> CollectedStatistics_;
};

}

#endif
