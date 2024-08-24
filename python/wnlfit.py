# -*- coding: utf-8 -*-
#
#  wnlfit.py
#
#  Copyright (C) 2024 Zhang Maiyun <maz005@ucsd.edu>
#   As a project developed during Maiyun's employment at the University of
#   California, The University of California may have claims to this work.
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <https://www.gnu.org/licenses/>.
#

"""Non-linear least squares fitting and error bar plotting."""

import numpy as np
from matplotlib import pyplot as plt
from scipy.optimize import curve_fit
from scipy.stats.distributions import t as tdistr


def nlpredci(modelfun, X, beta, R, covmatrix, alpha=0.05):
    """Nonlinear prediction with confidence intervals.

    Alternative implementation of `_nlpredci` with the help of GPT,
    unrelated to MATLAB's `nlpredci`.
    For the purpose of plotting a confidence interval around the prediction,
    this should agree enough with the MATLAB implementation.

    Parameters
    ----------
    modelfun : callable
        The model function.
    X : array_like
        Values of the independent variable.
    beta : array_like
        The parameters of the model.
    R : array_like
        The residuals from the fit.
    covmatrix : array_like
        The covariance matrix of the fit.

    Returns
    -------
    ypred : array_like
        The predicted values for each value of X.
    delta : array_like
        The uncertainty in the prediction.
    """
    def grad_at(x, modelfun, beta):
        eps = np.sqrt(np.finfo(float).eps)
        grad = np.zeros(len(beta))
        for i in range(len(beta)):
            delta = np.zeros_like(beta)
            delta[i] = eps * (1 + abs(beta[i]))
            grad[i] = (modelfun(beta + delta, x) -
                       modelfun(beta - delta, x)) / (2 * delta[i])
        return grad
    Ypred = modelfun(beta, X)
    # Degrees of freedom
    dof = max(0, len(R) - len(beta))
    mse = np.sum(R**2) / dof
    # Prediction variance using covariance matrix
    def jac(x): return grad_at(x, modelfun, beta)
    pred_var = np.array([mse * (1 + jac(x) @ covmatrix @ jac(x)) for x in X])
    t_stat = tdistr.ppf(1 - alpha / 2, dof)
    # Calculate the confidence interval half-widths
    delta = t_stat * np.sqrt(pred_var)
    return Ypred, delta


def wnlfit(x, y, xe, ye, model_fun, start):
    """Uses non-linear regression to fit a model to data.

    Translated from the MATLAB implementation by Andy Briggs, April 14, 2011,
    provided for the UCSD PHYS 2CL course.

    Parameters
    ----------
    x : array_like
        Independent variable.
    y : array_like
        Dependent variable.
    xe : array_like
        Uncertainty in the independent variable.
    ye : array_like
        Uncertainty in the dependent variable.
    model_fun : callable
        Model function (R^n, x) -> y.
    start : array_like
        Initial guess for the n parameters of the model.

    Returns
    -------
    params : array
        The n parameters of the model that best fit the data.
    params_err : array
        The standard error of the parameters.
    ypred : array
        The predicted values of 1000 points in the range of the input x.
    ypred_conf : array
        Half-width of the 95% confidence interval for each of the 1000 points in ypred.
    """
    if len(x) != len(y):
        raise ValueError("x and y must have the same length")
    if not len(xe) and not len(ye):
        w = np.ones(len(x))
    elif not len(ye):
        # WHY
        w = abs(1 / xe)
    elif not len(xe):
        w = abs(1 / ye)
    else:
        w = 1 / np.sqrt(xe**2 + ye**2)
    yw = np.sqrt(w) * y

    def modelfunw(x, *args):
        params = np.array(args)
        return np.sqrt(w) * model_fun(params, x)
    # https://www.mathworks.com/help/stats/nlinfit.html
    # `scipy`'s `curve_fit` seems to use the same iterative Levenberg-Marquardt least squares as MATLAB's `nlinfit`.
    popt, pcov = curve_fit(modelfunw, x, yw, p0=start)
    res = y - model_fun(popt, x)
    sefitw = np.sqrt(np.diag(pcov))
    for n, (p, se) in enumerate(zip(popt, sefitw)):
        print(f"Parameter {n+1}: p[{n}] = {p:6e} +/- {se:1e}")
    # The rest are for plotting and computing the 95% CI over the range of x. Unused in 2CL.
    # Why does the original code use the covariance matrix instead of the Jacobian?
    # But anyways it makes my life easier here.
    xgrid = np.linspace(min(x), max(x), 1000)
    ypred, ypred_conf = nlpredci(model_fun, xgrid, popt, res, pcov)
    plt.errorbar(x, y, yerr=ye, xerr=xe, fmt="none", capsize=1)
    plt.plot(xgrid, ypred, color="grey", lw=0.5)
    plt.fill_between(xgrid, ypred - ypred_conf, ypred +
                     ypred_conf, alpha=0.1, color="grey")
    return popt, sefitw, ypred, ypred_conf


class TestCases:
    # Test case
    # model = @(p,T) p(1)*exp(-T/p(2))+p(3)
    # ev = 1e-4
    T = np.arange(0, 0.00451, 0.0005)
    V = np.array([
        0.848540000000000,
        0.538720000000000,
        0.423000000000000,
        0.377100000000000,
        0.360250000000000,
        0.352200000000000,
        0.352440000000000,
        0.350490000000000,
        0.350000000000000,
        0.353170000000000,
    ])

    @classmethod
    def test_exp_2cl(cls):
        # d = wnlfit(t,v,zeros(size(t)),ev*ones(size(t)),model,[0.2 0.0001 1])
        # d.param
        expected_params = np.array([
            0.498100139345142,
            0.000515116892502,
            0.350407774505273,
        ])
        # d.paramerr
        expected_se = np.array([
            0.001501676542754,
            0.000003753954690,
            0.000603713637235
        ])
        # d.fit(1:100:size(d.fit))
        expected_ypred = np.array([
            0.848507913850415,
            0.558158422488270,
            0.437057683485621,
            0.386548247708115,
            0.365481463776217,
            0.356694800880431,
            0.353030005860282,
            0.351501470772615,
            0.350863940023587,
            0.350598034805424,
        ])
        # d.fitconf(1:100:size(d.fit))
        expected_ypred_conf = np.array([
            0.003339643332236,
            0.002627407494508,
            0.002083286528765,
            0.001396443960309,
            0.001169022088062,
            0.001222586671475,
            0.001309265126712,
            0.001366658230915,
            0.001397845544554,
            0.001413487738077,
        ])
        terr = np.zeros_like(cls.T)
        verr = 0.0001 * np.ones_like(cls.T)
        def model(p, T): return p[0] * np.exp(-T / p[1]) + p[2]
        d = wnlfit(cls.T, cls.V, terr, verr, model, [0.2, 0.0001, 1])
        assert np.allclose(d[0], expected_params, rtol=1e-6)
        assert np.allclose(d[1], expected_se, rtol=1e-6)
        assert np.allclose(d[2][::100], expected_ypred, rtol=1e-6, atol=1e-8)
        assert np.allclose(
            d[3][::100], expected_ypred_conf, rtol=1e-6, atol=1e-4)

    @classmethod
    def test_exp_pathological_uncert_1(cls):
        # d = wnlfit(t,v,0.001*ones(size(t)),ev*(1:size(t))',model,[0.2 0.0001 1])
        # d.param
        expected_params = np.array([
            0.498156387436208,
            0.000515381579298,
            0.350337069510079,
        ])
        # d.paramerr
        expected_se = np.array([
            0.001356346839086,
            0.000003452638634,
            0.000597164015365
        ])
        # d.fit(1:100:size(d.fit))
        expected_ypred = np.array([
            0.848493456946287,
            0.558204510434532,
            0.437074636439328,
            0.386530354438911,
            0.365439566851377,
            0.356638940876339,
            0.352966673198755,
            0.351434333347694,
            0.350794928591476,
            0.350528121954561,
        ])
        # d.fitconf(1:100:size(d.fit))
        expected_ypred_conf = np.array([
            0.002969207558983,
            0.002376559228780,
            0.001879011121817,
            0.001282587264768,
            0.001126508630329,
            0.001204869931671,
            0.001295588171143,
            0.001352692974855,
            0.001383218832653,
            0.001398430686399,
        ])
        terr = 0.001 * np.ones_like(cls.T)
        verr = 0.0001 * np.arange(1, len(cls.T)+1)
        def model(p, T): return p[0] * np.exp(-T / p[1]) + p[2]
        d = wnlfit(cls.T, cls.V, terr, verr, model, [0.2, 0.0001, 1])
        assert np.allclose(d[0], expected_params, rtol=1e-6)
        assert np.allclose(d[1], expected_se, rtol=1e-6)
        assert np.allclose(d[2][::100], expected_ypred, rtol=1e-6, atol=1e-8)
        assert np.allclose(
            d[3][::100], expected_ypred_conf, rtol=1e-6, atol=1e-4)

    @classmethod
    def test_exp_pathological_uncert_2(cls):
        # d = wnlfit(t,v,ev*((size(t):-1:1).^3)',ev*(1:size(t))',model,[0.2 0.0001 1])
        # d.param
        expected_params = np.array([
            0.498081412877021,
            0.000508738461708,
            0.351229540539077,
        ])
        # d.paramerr
        expected_se = np.array([
            0.009377864273763,
            0.000016492390420,
            0.000627195895286
        ])
        # d.fit(1:100:size(d.fit))
        expected_ypred = np.array([
            0.849310953416098,
            0.556707170490855,
            0.435996920567820,
            0.386199326391793,
            0.365655914483514,
            0.357180971470375,
            0.353684733387011,
            0.352242401477922,
            0.351647384409183,
            0.351401917114270,
        ])
        # d.fitconf(1:100:size(d.fit))
        expected_ypred_conf = np.array([
            0.022205037659838,
            0.013104810337876,
            0.010329402132224,
            0.006430951442363,
            0.003608287556592,
            0.002095558699185,
            0.001536897520086,
            0.001431224951611,
            0.001440370068526,
            0.001459313398933,
        ])
        terr = 0.0001 * (np.arange(len(cls.T), 0, -1)**3)
        verr = 0.0001 * np.arange(1, len(cls.T)+1)
        def model(p, T): return p[0] * np.exp(-T / p[1]) + p[2]
        d = wnlfit(cls.T, cls.V, terr, verr, model, [0.2, 0.0001, 1])
        assert np.allclose(d[0], expected_params, rtol=1e-6)
        assert np.allclose(d[1], expected_se, rtol=1e-6)
        assert np.allclose(d[2][::100], expected_ypred, rtol=1e-6, atol=1e-8)
        assert np.allclose(
            d[3][::100], expected_ypred_conf, rtol=1e-6, atol=1e-4)
