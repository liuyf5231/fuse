/*
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2018, Locus Robotics
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of the copyright holder nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef FUSE_CORE_UTIL_H
#define FUSE_CORE_UTIL_H

#include <ros/console.h>
#include <ros/node_handle.h>

#include <fuse_core/eigen.h>

#include <ceres/jet.h>
#include <Eigen/Core>

#include <cmath>
#include <string>
#include <vector>


namespace fuse_core
{

/**
 * @brief Returns the Euler pitch angle from a quaternion
 *
 * @param[in] w The quaternion real-valued component
 * @param[in] x The quaternion x-axis component
 * @param[in] y The quaternion x-axis component
 * @param[in] z The quaternion x-axis component
 * @return      The quaternion's Euler pitch angle component
 */
template <typename T>
static inline T getPitch(const T w, const T x, const T y, const T z)
{
  // Adapted from https://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles
  const T sin_pitch = T(2.0) * (w * y - z * x);

  if (ceres::abs(sin_pitch) >= T(1.0))
  {
    return (sin_pitch >= T(0.0) ? T(1.0) : T(-1.0)) * T(M_PI / 2.0);
  }
  else
  {
    return ceres::asin(sin_pitch);
  }
}

/**
 * @brief Returns the Euler roll angle from a quaternion
 *
 * @param[in] w The quaternion real-valued component
 * @param[in] x The quaternion x-axis component
 * @param[in] y The quaternion x-axis component
 * @param[in] z The quaternion x-axis component
 * @return      The quaternion's Euler pitch angle component
 */
template <typename T>
static inline T getRoll(const T w, const T x, const T y, const T z)
{
  // Adapted from https://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles
  const T sin_roll = T(2.0) * (w * x + y * z);
  const T cos_roll = T(1.0) - (T(2.0) * (x * x + y * y));
  return ceres::atan2(sin_roll, cos_roll);
}

/**
 * @brief Returns the Euler yaw angle from a quaternion
 *
 * @param[in] w The quaternion real-valued component
 * @param[in] x The quaternion x-axis component
 * @param[in] y The quaternion x-axis component
 * @param[in] z The quaternion x-axis component
 * @return      The quaternion's Euler pitch angle component
 */
template <typename T>
static inline T getYaw(const T w, const T x, const T y, const T z)
{
  // Adapted from https://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles
  const T sin_yaw = T(2.0) * (w * z + x * y);
  const T cos_yaw = T(1.0) - (T(2.0) * (y * y + z * z));
  return ceres::atan2(sin_yaw, cos_yaw);
}

/**
 * @brief Wrap a 2D angle to the standard (-Pi, +Pi] range.
 *
 * @param[in/out] angle Input angle to be wrapped to the (-Pi, +Pi] range. Angle is updated by this function.
 */
template <typename T>
void wrapAngle2D(T& angle)
{
  // Define some necessary variations of PI with the correct type (double or Jet)
  static const T PI = T(M_PI);
  static const T TAU = T(2 * M_PI);
  // Handle the 1*Tau roll-over (https://tauday.com/tau-manifesto)
  // Use ceres::floor because it is specialized for double and Jet types.
  angle -= TAU * ceres::floor((angle + PI) / TAU);
}

/**
 * @brief Wrap a 2D angle to the standard (-Pi, +Pi] range.
 *
 * @param[in] angle Input angle to be wrapped to the (-Pi, +Pi] range.
 * @return The equivalent wrapped angle
 */
template <typename T>
T wrapAngle2D(const T& angle)
{
  T wrapped = angle;
  wrapAngle2D(wrapped);
  return wrapped;
}

/**
 * @brief Create an 2x2 rotation matrix from an angle
 *
 * @param[in] angle The rotation angle, in radians
 * @return          The equivalent 2x2 rotation matrix
 */
template <typename T>
Eigen::Matrix<T, 2, 2, Eigen::RowMajor> rotationMatrix2D(const T angle)
{
  const T cos_angle = ceres::cos(angle);
  const T sin_angle = ceres::sin(angle);
  Eigen::Matrix<T, 2, 2, Eigen::RowMajor> rotation;
  rotation << cos_angle, -sin_angle, sin_angle, cos_angle;
  return rotation;
}


/**
 * @brief Helper function that loads strictly positive integral or floating point values from the parameter server
 *
 * @param[in] node_handle - The node handle used to load the parameter
 * @param[in] parameter_name - The parameter name to load
 * @param[in] default_value - A default value to use if the provided parameter name does not exist
 * @return The loaded (or default) value
 */
template <typename T,
          typename std::enable_if<std::is_integral<T>::value || std::is_floating_point<T>::value>::type* = nullptr>
T getPositiveParam(const ros::NodeHandle& node_handle, const std::string& parameter_name, T default_value)
{
  T value;
  node_handle.param(parameter_name, value, default_value);
  if (value <= 0)
  {
    ROS_WARN_STREAM("The requested " << parameter_name << " is <= 0. Using the default value (" <<
                    default_value << ") instead.");
    value = default_value;
  }
  return value;
}

/**
 * @brief Helper function that loads a covariance matrix diagonal vector from the parameter server and checks the size
 * and the values are invalid, i.e. they are positive.
 *
 * @tparam Scalar - A scalar type, defaults to double
 * @tparam Size - An int size that specifies the expected size of the covariance matrix (rows and columns)
 *
 * @param[in] node_handle - The node handle used to load the parameter
 * @param[in] parameter_name - The parameter name to load
 * @param[in] default_value - A default value to use for all the diagonal elements if the provided parameter name does
 *                            not exist
 * @return The loaded (or default) covariance matrix, generated from the diagonal vector
 */
template <int Size, typename Scalar = double>
fuse_core::Matrix<Scalar, Size, Size> getCovarianceDiagonalParam(const ros::NodeHandle& node_handle,
                                                                 const std::string& parameter_name,
                                                                 Scalar default_value)
{
  using Vector = typename Eigen::Matrix<Scalar, Size, 1>;

  std::vector<Scalar> diagonal(Size, default_value);
  node_handle.param(parameter_name, diagonal, diagonal);

  const auto diagonal_size = diagonal.size();
  if (diagonal_size != Size)
  {
    throw std::invalid_argument("Invalid size of " + std::to_string(diagonal_size) + ", expected " +
                                std::to_string(Size));
  }

  if (std::any_of(diagonal.begin(), diagonal.end(),
                  [](const auto& value) { return value < Scalar(0); }))  // NOLINT(whitespace/braces)
  {
    throw std::invalid_argument("Invalid negative diagonal values in " + fuse_core::to_string(Vector(diagonal.data())));
  }

  return Vector(diagonal.data()).asDiagonal();
}

}  // namespace fuse_core

#endif  // FUSE_CORE_UTIL_H
