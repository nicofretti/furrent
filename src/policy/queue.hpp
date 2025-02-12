/**
 * @file queue.hpp
 * @author Filippo Ziche (filippo.ziche@gmail.com)
 * @version 0.1
 * @date 2022-09-05
 *
 * @copyright Copyright (c) 2022
 *
 */

#pragma once

#include <optional>
#include <policy/policy.hpp>
#include <util/result.hpp>

namespace fur::policy {

/// @brief Special queue that allows the extraction of item using custom
/// policies
/// @tparam T type of the stored items
template <typename T>
class Queue {
  /// Stored items
  std::list<T> _items;

 public:
  /// All possible error that can occur
  enum class Error {
    /// There are no more elements
    Empty,
    /// Policy returned no element
    PolicyFailure
  };

  /// Custom queue result type
  typedef util::Result<T, Error> Result;

  // Function used to mutate the internal collection
  // returns false if the element should be kept, true otherwise
  typedef std::function<bool(T&)> MutateFn;

 public:
  /// Insert a new element
  /// @param item element to be inserted
  void insert(T&& item);

  /// Construct and insert a new element
  /// @param ...args arguments list used in constructor of T
  template <typename... Args>
  void emplace(Args&&... args);

  /// Tries to extract an element using a policy
  /// @param policy custom logic used to select the element
  [[nodiscard]] Result extract(const IPolicy<T>& policy);

  /// Mutate the internal list of items
  void mutate(MutateFn mutation);

  /// @return inner list of items
  std::list<T>& items();

  /// @return Number of items present
  [[nodiscard]] size_t size() const;
};

}  // namespace fur::policy

#include <policy/queue.inl>