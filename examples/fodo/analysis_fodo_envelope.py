#!/usr/bin/env python3
#
# Copyright 2022-2023 ImpactX contributors
# Authors: Axel Huebl, Chad Mitchell
# License: BSD-3-Clause-LBNL
#

import glob
import re

import numpy as np
import pandas as pd


def read_file(file_pattern):
    for filename in glob.glob(file_pattern):
        df = pd.read_csv(filename, delimiter=r"\s+")
        if "step" not in df.columns:
            step = int(re.findall(r"[0-9]+", filename)[0])
            df["step"] = step
        yield df


def read_time_series(file_pattern):
    """Read in all CSV files from each MPI rank (and potentially OpenMP
    thread). Concatenate into one Pandas dataframe.

    Returns
    -------
    pandas.DataFrame
    """
    return pd.concat(
        read_file(file_pattern),
        axis=0,
        ignore_index=True,
    )  # .set_index('id')


# read reduced diagnostics
rbc = read_time_series("diags/reduced_beam_characteristics.*")

s = rbc["s"]
sig_x = rbc["sig_x"]
sig_y = rbc["sig_y"]
sig_t = rbc["sig_t"]
emittance_x = rbc["emittance_x"]
emittance_y = rbc["emittance_y"]
emittance_t = rbc["emittance_t"]

sig_xi = sig_x.iloc[0]
sig_yi = sig_y.iloc[0]
sig_ti = sig_t.iloc[0]
emittance_xi = emittance_x.iloc[0]
emittance_yi = emittance_y.iloc[0]
emittance_ti = emittance_t.iloc[0]

length = len(s) - 1

sf = s.iloc[length]
sig_xf = sig_x.iloc[length]
sig_yf = sig_y.iloc[length]
sig_tf = sig_t.iloc[length]
emittance_xf = emittance_x.iloc[length]
emittance_yf = emittance_y.iloc[length]
emittance_tf = emittance_t.iloc[length]


print("Initial Beam:")
print(f"  sigx={sig_xi:e} sigy={sig_yi:e} sigt={sig_ti:e}")
print(
    f"  emittance_x={emittance_xi:e} emittance_y={emittance_yi:e} emittance_t={emittance_ti:e}"
)

atol = 0.0  # ignored
rtol = 1.0e-2  # from random sampling of a smooth distribution
print(f"  rtol={rtol} (ignored: atol~={atol})")

assert np.allclose(
    [sig_xi, sig_yi, sig_ti, emittance_xi, emittance_yi, emittance_ti],
    [
        7.5451170454175073e-005,
        7.5441588239210947e-005,
        9.9775878164077539e-004,
        1.9959540393751392e-009,
        2.0175015289132990e-009,
        2.0013820193294972e-006,
    ],
    rtol=rtol,
    atol=atol,
)


print("")
print("Final Beam:")
print(f"  sigx={sig_xf:e} sigy={sig_yf:e} sigt={sig_tf:e}")
print(
    f"  emittance_x={emittance_xf:e} emittance_y={emittance_yf:e} emittance_t={emittance_tf:e}"
)

atol = 0.0  # ignored
rtol = 1.0e-2  # from random sampling of a smooth distribution
print(f"  rtol={rtol} (ignored: atol~={atol})")

assert np.allclose(
    [sig_xf, sig_yf, sig_tf, emittance_xf, emittance_yf, emittance_tf],
    [
        7.4790118496224206e-005,
        7.5357525169680140e-005,
        9.9775879288128088e-004,
        1.9959539836392703e-009,
        2.0175014668882125e-009,
        2.0013820380883801e-006,
    ],
    rtol=rtol,
    atol=atol,
)
