#include "pybind_main.h"

/**
 * Given a pybind11::iterable, return a stream.
 * @param data to create the stream from.
 * @param caller string representing the name of the iterable.
 */
std::stringstream GetStream(const py::iterable &data, const std::string &caller = "") {
  std::stringstream out;
  for (const auto &line : data)
    if (py::isinstance<py::str>(line))
      out << line.cast<std::string>() << std::endl;
    else
      throw ERROR_MSG(caller, " must be a list of strings.");
  return out;
}

/**
 * Given options in pybind11::dict format, load the corresponding data to data_type.
 * @param options in pybind11::dict format.
 * @param argument key corresponding to the desired option.
 * @param data object where to upload the option to.
 */
template<typename data_type> void LoadData(const py::dict &options,
    const std::string &argument, data_type &data) {
  if (options.contains(argument.c_str())) {
    const auto &data_iterable = options[argument.c_str()];
    if (py::isinstance<py::iterable>(data_iterable))
      data.load(GetStream(data_iterable.cast<py::iterable>(), argument));
    else
      throw ERROR_MSG("'", argument, "' must be a list of strings.");
  } else if (options.contains((argument + "_filename").c_str())) {
    const auto &filename = options[(argument + "_filename").c_str()];
    if (py::isinstance<py::str>(filename))
      data.load(filename.cast<std::string>());
    else
      throw ERROR_MSG("'", argument, "_filename' must be a string.");
  }
}

/**
 * Given options in pybind11::dict format, load states.
 * @param options in pybind11::dict format.
 * @param argument key corresponding to the desired option.
 * @param number of active qubits.
 */
std::vector<std::string> LoadStates(const py::dict &options, const std::string &argument,
    const std::size_t num_active_qubits = 0) {
  std::vector<std::string> states;
  if (options.contains((argument + "_states").c_str())) {
    const auto &in_states = options[(argument + "_states").c_str()];
    if (py::isinstance<py::str>(in_states)) {
      states.push_back(in_states.cast<std::string>());
    } else if (py::isinstance<py::iterable>(in_states)) {
      for (const auto &x : in_states.cast<py::iterable>())
        if (py::isinstance<py::str>(x))
          states.push_back(x.cast<std::string>());
        else
          throw ERROR_MSG("states must be strings.");
    } else
      throw ERROR_MSG(argument, "_states must be a list of strings.");
  } else if (options.contains((argument + "_state").c_str())) {
    const auto &in_state = options[(argument + "_state").c_str()];
    if (py::isinstance<py::str>(in_state)) {
      states.push_back(in_state.cast<std::string>());
    } else
      throw ERROR_MSG("states must be strings.");
  } else {
    if (argument == "initial")
      states.push_back(std::string(num_active_qubits, '0'));
    else
      states.push_back("");
  }
  return states;
}

std::vector<std::pair<std::string, std::complex<double>>> simulate(
    const py::dict &options) {
  qflex::QflexInput input;

  try {
    // Get qFlex input
    qflex::QflexInput input;

    // Load grid, circuit and ordering
    LoadData(options, "grid", input.grid);
    LoadData(options, "circuit", input.circuit);
    LoadData(options, "ordering", input.ordering);
    const auto initial_states =
        LoadStates(options, "initial", input.circuit.num_active_qubits);
    const auto final_states = LoadStates(options, "final");

    // Define container for amplitudes
    std::vector<std::pair<
        std::string, std::vector<std::pair<std::string, std::complex<double>>>>>
        amplitudes;

    // TODO: Pybind11 does not allow multiple types as output
    if (std::size(initial_states) != 1 or std::size(final_states) != 1)
      throw ERROR_MSG("Not yet supported");

    for (const auto &is : initial_states) {
      for (const auto &fs : final_states) {
        input.initial_state = is;
        input.final_state = fs;
        amplitudes.push_back({is, EvaluateCircuit(&input)});
      }
    }

    if (std::size(initial_states) == 1 and std::size(final_states) == 1)
      return std::get<1>(amplitudes[0]);
    else {
      return {};
      // return amplitudes;
    }

  } catch (std::string err_msg) {
    std::cerr << err_msg << std::endl;
    return {};
  } catch (const char *err_msg) {
    std::cerr << err_msg << std::endl;
    return {};
  } catch (...) {
    std::cerr << "Something went wrong" << std::endl;
    return {};
  }
}
