//
// Created by Z1ko on 28/07/2022.
//

#pragma once

#include <optional>
#include <condition_variable>

namespace fur::mt {

/// Given a collection of type C extracts a work item.
template<typename To, typename C>
class IRouterStrategy {
public:
  virtual ~IRouterStrategy() = default;
  /// This function is guaranteed to be thread-safe.
  virtual std::optional<To> operator() (C& collection) = 0;
};

template<typename From, typename To>
using IVectorRouterStrategy = IRouterStrategy<To, std::vector<From>>;

/// Represents a synchronization object used to orchestrate the
/// distribution of work items to threads, it owns the items it serves.
/// @tparam From Type of the items inside the collection
/// @tparam To   Type of the work-items produced
/// @tparam Col  Type of the collection containing the items
template<typename From, typename To, typename C>
class IRouter {
public:
  virtual ~IRouter() = default;

  /// Insert a work item inside the collection in a thread-safe manner
  /// @param item The items to be inserted
  virtual void insert(From&& item) = 0;

  /// Wait for an available work item
  virtual std::optional<To> get_work() = 0;

  /// Returns the amount of work items present at the moment
  virtual size_t size() = 0;

  /// Stops serving work items to threads,
  /// wakes up all waiting threads returning a nullopt work item,
  /// used to return control of the threads to the outside
  virtual void stop() = 0;

  /// Resume serving all threads entering the router
  virtual void resume() = 0;

  /// Blocks until there is no more work to do
  virtual void busy() = 0;
};

template<typename From, typename To>
using IVectorRouter = IRouter<From, To, std::vector<From>>;

/// Router implementation using a vector as the underlying collection
template<typename From, typename To>
class VectorRouter : public IVectorRouter<From, To> {

protected:
 /// Mutex protecting all internal state
  std::mutex m_mutex;
  /// CV used to signal that new work is available
  std::condition_variable m_work_available;
  /// CV used to signal that there is no more work available
  std::condition_variable m_work_finished;
  /// True if workers should be served new work
  bool m_should_serve;

  /// Strategy that will be used to extract work from the collection
  std::unique_ptr<IVectorRouterStrategy<From, To>> m_strategy;
  /// Collection with work items to be distributed
  std::vector<From> m_work_items;

public:
  /// Construct a new router with a strategy
  /// @param strategy The strategy to be used, router takes ownership of the object
  explicit VectorRouter(std::unique_ptr<IVectorRouterStrategy<From, To>> strategy);

  void insert(From&& item) override;
  [[nodiscard]] std::optional<To> get_work() override;

  /// Changes the strategy to be used in selecting the work-items
  /// @param strategy The new strategy to be used, router takes ownership of the object
  void set_strategy(std::unique_ptr<IVectorRouterStrategy<From, To>> strategy);

  void   stop()   override;
  void   resume() override;
  void   busy()   override;
  size_t size()   override;
};

} // namespace fur::mt

#include <mt/router.inl>