# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.

import math
import os
import unittest

from main import PipelineSession, run


class PipelineTests(unittest.TestCase):
    def test_cpu_or_opencl_processing(self):
        _, backend, result = run([1.0, -2.0, 3.5], 2.0)
        self.assertIn(backend, {"CPU", "GPU", "OPENCL_CPU", "HYBRID"})
        if os.environ.get("PIPELINE_REQUIRE_OPENCL") == "1":
            self.assertIn(backend, {"GPU", "OPENCL_CPU", "HYBRID"},
                          "CI OpenCL job caiu no fallback CPU; kernel nao foi validado")
        self.assertEqual(result, [2.0, -4.0, 7.0])

    def test_fractional_scale(self):
        _, _, result = run([2.0, 4.0], 0.5)
        self.assertEqual(result, [1.0, 2.0])

    def test_zero_scale(self):
        _, _, result = run([10.0, -10.0], 0.0)
        self.assertEqual(result, [0.0, 0.0])

    def test_empty_input_rejected(self):
        with self.assertRaises(ValueError):
            run([], 1.0)



    def test_async_submit_and_collect(self):
        with PipelineSession() as session:
            tasks = [
                (session.submit([1.0, 2.0, 3.0], 2.0), [2.0, 4.0, 6.0]),
                (session.submit([-1.0, 5.0], 3.0), [-3.0, 15.0]),
                (session.submit([0.5, 4.0], -2.0), [-1.0, -8.0]),
            ]
            # As tarefas sao submetidas antes da coleta: a API nao e mais
            # limitada a uma unica chamada sincrona por vez.
            self.assertEqual(session.collect(tasks[1][0]), tasks[1][1])
            self.assertEqual(session.collect(tasks[0][0]), tasks[0][1])
            self.assertEqual(session.collect(tasks[2][0]), tasks[2][1])


    def test_async_pending_limit_is_bounded(self):
        with PipelineSession() as session:
            tasks = [session.submit([float(i)], 2.0) for i in range(64)]
            self.assertEqual(len(tasks), 64)
            with self.assertRaises(RuntimeError):
                session.submit([99.0], 2.0)
            for i, task_id in enumerate(tasks):
                self.assertEqual(session.collect(task_id), [float(i) * 2.0])

    def test_negative_scale(self):
        _, _, result = run([1.5, -2.0], -2.0)
        self.assertEqual(result, [-3.0, 4.0])

    def test_large_cpu_fallback_result_shape(self):
        values = [float(i % 17) for i in range(12000)]
        _, backend, result = run(values, 1.25)
        self.assertEqual(len(result), len(values))
        self.assertEqual(result[0], 0.0)
        self.assertAlmostEqual(result[1], 1.25)
        self.assertAlmostEqual(result[-1], values[-1] * 1.25)
        self.assertIn(backend, {"CPU", "GPU", "OPENCL_CPU", "HYBRID"})

    def test_non_finite_scale_rejected(self):
        with self.assertRaises(RuntimeError):
            run([1.0], math.inf)


if __name__ == "__main__":
    unittest.main()
