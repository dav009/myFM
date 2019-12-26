#ifndef MYFM_DECLARE_MODULE_HPP
#define MYFM_DECLARE_MODULE_HPP

#include <random>
#include <tuple>
#include <vector>

#include "FM.hpp"
#include "FMLearningConfig.hpp"
#include "FMTrainer.hpp"
#include "definitions.hpp"
#include <functional>
#include <iostream>

#include <pybind11/eigen.h>
#include <pybind11/functional.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

using namespace std;

namespace py = pybind11;
template<typename Real>
std::pair<std::vector<myFM::FM<Real>>, std::vector<myFM::FMHyperParameters<Real>>>
create_train_fm(size_t n_factor, Real init_std, const typename myFM::FM<Real>::SparseMatrix &X,
                const typename myFM::FM<Real>::Vector &y, int random_seed, myFM::FMLearningConfig<Real> &config,
                std::function<bool(int, const myFM::FM<Real> &, const myFM::FMHyperParameters<Real> &)> cb) {
  myFM::FMTrainer<Real> fm_trainer(X, y, random_seed, config);
  auto fm = fm_trainer.create_FM(n_factor, init_std);
  auto hyper_param = fm_trainer.create_Hyper(fm.n_factors);
  return fm_trainer.learn_with_callback(fm, hyper_param, cb);
}

template<typename Real>
void declare_functional(py::module & m) {
  using FMTrainer = myFM::FMTrainer<Real>;
  using FM = myFM::FM<Real>;
  using Hyper = myFM::FMHyperParameters<Real>;
  using SparseMatrix = typename FM::SparseMatrix;
  using FMLearningConfig = typename myFM::FMLearningConfig<Real>;
  using Vector = typename FM::Vector;
  using DenseMatrix = typename FM::DenseMatrix;
  using ConfigBuilder = typename FMLearningConfig::Builder;


  m.doc() = "Backend C++ inplementation for myfm.";

  py::enum_<typename FMTrainer::TASKTYPE>(m, "TaskType", py::arithmetic())
    .value("REGRESSION", FMTrainer::TASKTYPE::REGRESSION)
    .value("CLASSIFICATION", FMTrainer::TASKTYPE::CLASSIFICATION);

  py::class_<FMLearningConfig>(m, "FMLearningConfig");

  py::class_<ConfigBuilder>(m, "ConfigBuilder")
    .def(py::init<>())
    .def("set_alpha_0", &ConfigBuilder::set_alpha_0)
    .def("set_beta_0", &ConfigBuilder::set_beta_0)
    .def("set_gamma_0", &ConfigBuilder::set_gamma_0)
    .def("set_mu_0", &ConfigBuilder::set_mu_0)
    .def("set_reg_0", &ConfigBuilder::set_reg_0)
    .def("set_n_iter", &ConfigBuilder::set_n_iter)
    .def("set_n_kept_samples", &ConfigBuilder::set_n_kept_samples)
    .def("set_task_type", &ConfigBuilder::set_task_type)
    .def("set_group_index", &ConfigBuilder::set_group_index)
    .def("set_indentical_groups", &ConfigBuilder::set_indentical_groups)
    .def("build", &ConfigBuilder::build);

  py::class_<FM>(m, "FM")
    .def_readwrite("w0", &FM::w0)
    .def_readwrite("w", &FM::w)
    .def_readwrite("V", &FM::V)
    .def("__getstate__",
        [](const FM &fm) {
        Real w0 = fm.w0;
        Vector w(fm.w);
        DenseMatrix V(fm.V);
        return py::make_tuple(w0, w, V);
        })
  .def("__setstate__", [](FM &fm, py::tuple t) {
      if (t.size() != 3)
      throw std::runtime_error("invalid state for FM.");
      // placement new
      new (&fm) FM(t[0].cast<Real>(), t[1].cast<Vector>(),
          t[2].cast<DenseMatrix>());
      });

  py::class_<Hyper>(m, "FMHyperParameters")
    .def_readonly("alpha", &Hyper::alpha)
    .def_readonly("mu_w", &Hyper::mu_w)
    .def_readonly("lambda_w", &Hyper::lambda_w)
    .def_readonly("mu_V", &Hyper::mu_V)
    .def_readonly("lambda_V", &Hyper::lambda_V)
    .def("__getstate__",
        [](const Hyper &hyper) {
        Real alpha = hyper.alpha;
        Vector mu_w(hyper.mu_w);
        Vector lambda_w(hyper.lambda_V);
        DenseMatrix mu_V(hyper.mu_w);
        DenseMatrix lambda_V(hyper.lambda_V);
        return py::make_tuple(alpha, mu_w, lambda_w, mu_V, lambda_V);
        })
  .def("__setstate__", [](Hyper &hyper, py::tuple t) {
      if (t.size() != 5)
      throw std::runtime_error("invalid state for FMHyperParameters.");
      // placement new
      new (&hyper)
      Hyper(t[0].cast<Real>(), t[1].cast<Vector>(), t[2].cast<Vector>(),
          t[3].cast<DenseMatrix>(), t[4].cast<DenseMatrix>());
      });

  py::class_<FMTrainer>(m, "FMTrainer")
    .def(py::init<const SparseMatrix &, const Vector &, int,
        FMLearningConfig>())
    .def("create_FM", &FMTrainer::create_FM)
    .def("create_Hyper", &FMTrainer::create_Hyper)
    .def("learn", &FMTrainer::learn);

  m.def("create_train_fm", &create_train_fm<Real>, "create and train fm.");

}
#endif