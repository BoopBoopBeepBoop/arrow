// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

// Eager evaluation convenience APIs for invoking common functions, including
// necessary memory allocations

#pragma once

#include "arrow/compute/function.h"
#include "arrow/datum.h"
#include "arrow/result.h"
#include "arrow/util/macros.h"
#include "arrow/util/visibility.h"

namespace arrow {

class Array;

namespace compute {

class ExecContext;

// ----------------------------------------------------------------------
// Aggregate functions

/// \addtogroup compute-concrete-options
/// @{

/// \brief Control Count kernel behavior
///
/// By default, all non-null values are counted.
struct ARROW_EXPORT CountOptions : public FunctionOptions {
  enum Mode {
    /// Count all non-null values.
    COUNT_NON_NULL = 0,
    /// Count all null values.
    COUNT_NULL,
  };

  explicit CountOptions(enum Mode count_mode = COUNT_NON_NULL) : count_mode(count_mode) {}

  static CountOptions Defaults() { return CountOptions(COUNT_NON_NULL); }

  enum Mode count_mode;
};

/// \brief Control MinMax kernel behavior
///
/// By default, null values are ignored
struct ARROW_EXPORT MinMaxOptions : public FunctionOptions {
  enum Mode {
    /// Skip null values
    SKIP = 0,
    /// Any nulls will result in null output
    EMIT_NULL
  };

  explicit MinMaxOptions(enum Mode null_handling = SKIP) : null_handling(null_handling) {}

  static MinMaxOptions Defaults() { return MinMaxOptions{}; }

  enum Mode null_handling;
};

/// \brief Control Mode kernel behavior
///
/// Returns top-n common values and counts.
/// By default, returns the most common value and count.
struct ARROW_EXPORT ModeOptions : public FunctionOptions {
  explicit ModeOptions(int64_t n = 1) : n(n) {}

  static ModeOptions Defaults() { return ModeOptions{}; }

  int64_t n = 1;
};

/// \brief Control Delta Degrees of Freedom (ddof) of Variance and Stddev kernel
///
/// The divisor used in calculations is N - ddof, where N is the number of elements.
/// By default, ddof is zero, and population variance or stddev is returned.
struct ARROW_EXPORT VarianceOptions : public FunctionOptions {
  explicit VarianceOptions(int ddof = 0) : ddof(ddof) {}

  static VarianceOptions Defaults() { return VarianceOptions{}; }

  int ddof = 0;
};

/// \brief Control Quantile kernel behavior
///
/// By default, returns the median value.
struct ARROW_EXPORT QuantileOptions : public FunctionOptions {
  /// Interpolation method to use when quantile lies between two data points
  enum Interpolation {
    LINEAR = 0,
    LOWER,
    HIGHER,
    NEAREST,
    MIDPOINT,
  };

  explicit QuantileOptions(double q = 0.5, enum Interpolation interpolation = LINEAR)
      : q{q}, interpolation{interpolation} {}

  explicit QuantileOptions(std::vector<double> q,
                           enum Interpolation interpolation = LINEAR)
      : q{std::move(q)}, interpolation{interpolation} {}

  static QuantileOptions Defaults() { return QuantileOptions{}; }

  /// quantile must be between 0 and 1 inclusive
  std::vector<double> q;
  enum Interpolation interpolation;
};

/// @}

/// \brief Count non-null (or null) values in an array.
///
/// \param[in] options counting options, see CountOptions for more information
/// \param[in] datum to count
/// \param[in] ctx the function execution context, optional
/// \return out resulting datum
///
/// \since 1.0.0
/// \note API not yet finalized
ARROW_EXPORT
Result<Datum> Count(const Datum& datum, CountOptions options = CountOptions::Defaults(),
                    ExecContext* ctx = NULLPTR);

/// \brief Compute the mean of a numeric array.
///
/// \param[in] value datum to compute the mean, expecting Array
/// \param[in] ctx the function execution context, optional
/// \return datum of the computed mean as a DoubleScalar
///
/// \since 1.0.0
/// \note API not yet finalized
ARROW_EXPORT
Result<Datum> Mean(const Datum& value, ExecContext* ctx = NULLPTR);

/// \brief Sum values of a numeric array.
///
/// \param[in] value datum to sum, expecting Array or ChunkedArray
/// \param[in] ctx the function execution context, optional
/// \return datum of the computed sum as a Scalar
///
/// \since 1.0.0
/// \note API not yet finalized
ARROW_EXPORT
Result<Datum> Sum(const Datum& value, ExecContext* ctx = NULLPTR);

/// \brief Calculate the min / max of a numeric array
///
/// This function returns both the min and max as a struct scalar, with type
/// struct<min: T, max: T>, where T is the input type
///
/// \param[in] value input datum, expecting Array or ChunkedArray
/// \param[in] options see MinMaxOptions for more information
/// \param[in] ctx the function execution context, optional
/// \return resulting datum as a struct<min: T, max: T> scalar
///
/// \since 1.0.0
/// \note API not yet finalized
ARROW_EXPORT
Result<Datum> MinMax(const Datum& value,
                     const MinMaxOptions& options = MinMaxOptions::Defaults(),
                     ExecContext* ctx = NULLPTR);

/// \brief Test whether any element in a boolean array evaluates to true.
///
/// This function returns true if any of the elements in the array evaluates
/// to true and false otherwise. Null values are skipped.
///
/// \param[in] value input datum, expecting a boolean array
/// \param[in] ctx the function execution context, optional
/// \return resulting datum as a BooleanScalar
///
/// \since 3.0.0
/// \note API not yet finalized
ARROW_EXPORT
Result<Datum> Any(const Datum& value, ExecContext* ctx = NULLPTR);

/// \brief Test whether all elements in a boolean array evaluate to true.
///
/// This function returns true if all of the elements in the array evaluate
/// to true and false otherwise. Null values are skipped.
///
/// \param[in] value input datum, expecting a boolean array
/// \param[in] ctx the function execution context, optional
/// \return resulting datum as a BooleanScalar

/// \since 3.0.0
/// \note API not yet finalized
ARROW_EXPORT
Result<Datum> All(const Datum& value, ExecContext* ctx = NULLPTR);

/// \brief Calculate the modal (most common) value of a numeric array
///
/// This function returns top-n most common values and number of times they occur as
/// an array of `struct<mode: T, count: int64>`, where T is the input type.
/// Values with larger counts are returned before smaller ones.
/// If there are more than one values with same count, smaller value is returned first.
///
/// \param[in] value input datum, expecting Array or ChunkedArray
/// \param[in] options see ModeOptions for more information
/// \param[in] ctx the function execution context, optional
/// \return resulting datum as an array of struct<mode: T, count: int64>
///
/// \since 2.0.0
/// \note API not yet finalized
ARROW_EXPORT
Result<Datum> Mode(const Datum& value,
                   const ModeOptions& options = ModeOptions::Defaults(),
                   ExecContext* ctx = NULLPTR);

/// \brief Calculate the standard deviation of a numeric array
///
/// \param[in] value input datum, expecting Array or ChunkedArray
/// \param[in] options see VarianceOptions for more information
/// \param[in] ctx the function execution context, optional
/// \return datum of the computed standard deviation as a DoubleScalar
///
/// \since 2.0.0
/// \note API not yet finalized
ARROW_EXPORT
Result<Datum> Stddev(const Datum& value,
                     const VarianceOptions& options = VarianceOptions::Defaults(),
                     ExecContext* ctx = NULLPTR);

/// \brief Calculate the variance of a numeric array
///
/// \param[in] value input datum, expecting Array or ChunkedArray
/// \param[in] options see VarianceOptions for more information
/// \param[in] ctx the function execution context, optional
/// \return datum of the computed variance as a DoubleScalar
///
/// \since 2.0.0
/// \note API not yet finalized
ARROW_EXPORT
Result<Datum> Variance(const Datum& value,
                       const VarianceOptions& options = VarianceOptions::Defaults(),
                       ExecContext* ctx = NULLPTR);

/// \brief Calculate the quantiles of a numeric array
///
/// \param[in] value input datum, expecting Array or ChunkedArray
/// \param[in] options see QuantileOptions for more information
/// \param[in] ctx the function execution context, optional
/// \return resulting datum as an array
///
/// \since 4.0.0
/// \note API not yet finalized
ARROW_EXPORT
Result<Datum> Quantile(const Datum& value,
                       const QuantileOptions& options = QuantileOptions::Defaults(),
                       ExecContext* ctx = NULLPTR);

}  // namespace compute
}  // namespace arrow
