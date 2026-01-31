#include <pybind11/pybind11.h>
#include <pybind11/stl.h>  // ���������ͷ�ļ������� std::vector �޷�ת��
#include "GameEnv.h"

namespace py = pybind11;

// ���� Pybind11 ģ�飬ģ����������Ŀ���ɵ��ļ���һ��
PYBIND11_MODULE(GameEnv, m) {
    // 1. �� StepResult �ṹ�壬ʹ Python ���� res.reward ��ʽ����
    py::class_<StepResult>(m, "StepResult")
        .def_readonly("observation", &StepResult::observation)
        .def_readonly("reward", &StepResult::reward)
        .def_readonly("done", &StepResult::done)
        .def_readonly("score", &StepResult::score);

    // 2. �� GameEnv ������
    py::class_<GameEnv>(m, "GameEnv")
        // �󶨹��캯����Ĭ�� headless Ϊ true
        .def(py::init<bool>(), py::arg("headless") = true)

        // �󶨺����߼��ӿ�
        .def("reset", &GameEnv::reset)
        .def("step", &GameEnv::step, py::arg("action"))
        .def("get_obs", &GameEnv::get_obs)

        // �󶨽�����������ȡ�ӿڣ������� Python �˴�ӡ����
        .def("get_reward_pass", &GameEnv::get_reward_pass)
        .def("get_reward_death", &GameEnv::get_reward_death)
        .def("get_reward_hit", &GameEnv::get_reward_hit)
        .def("get_reward_step", &GameEnv::get_reward_step)
        .def("get_damage_taken", &GameEnv::get_damage_taken);
}