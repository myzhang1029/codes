(** User Mathematica initialization file **)

(* Always prefer metric units *)
$UnitSystem = "Metric"

(* Statistics *)

(* Derivative of `Abs` *)
Abs'[x_] := Sign[x] /; x != 0

(* Normal CDF *)
Phi[x_] := Erfc[-(x/Sqrt[2])]/2
Attributes[Phi] = {Listable, NumericFunction}

(* Compute relative error *)
RelativeError[x_Around] := x["Uncertainty"] / x["Value"]
RelativeError[q_Quantity] := RelativeError@QuantityMagnitude[q]
NormalTScore[expected_, actual_] := 1/RelativeError[expected - actual]

(* Quantity weighted by 1/sigma^2 *)
MeanAround2[data : {___Around}] := With[
  {d = #["Value"] & /@ data, w = #["Uncertainty"]^-2 & /@ data},
  Around[Total[d w]/Total[w], Total[w]^(-1/2)]
]
MeanAround2[data : {___Quantity}] := Quantity[MeanAround2@QuantityMagnitude@data, QuantityUnit@First@data]

(* Engineering *)

(* The hypotenuse of a right-angle triangle *)
Hypot[a_, b_] := Sqrt[a^2 + b^2]
Attributes[Hypot] = {Listable, NumericFunction}

(* Decibel conversion *)
dBm[dBm_] := Quantity[10^(dBm/10), "Milliwatts"]
(* dBV[dbv_] := First@SolveValues[
    20 Log[10, x/Quantity[1, "Volts"]] == dbv,
    x,
    Reals
] *)
dBV[dbv_] := Quantity[10^(dbv/20), "Volts"]
(* dBu[dbu_] := First@SolveValues[
    20 Log[10, x/Quantity[Sqrt[3/5], "Volts"]] == dbu,
    x,
    Reals
] *)
dBu[dbu_] := Quantity[10^(dbu/20) Sqrt[3/5], "Volts"]

(* Irregular-Time Discrete Fourier Transform *)
IDFT[(\[Omega]_)?(NumericQ[#] || QuantityQ[#] &), times_, vals_] :=
 Mean[vals E^(-I \[Omega] times)]
(* Convert voltage FT to dB *)
VFT2dB[x_] := 20 Log10@Abs[x]
(* Engineering-convention Fourier Transform *)
EFourierTransform[args___] := FourierTransform[args, FourierParameters -> {1, -1}]

(* The rect function *)
Rect[x_] := Piecewise[{
    {1, -(1/2) < x < 1/2},
    {1/2, Abs[x] == 1/2},
    {0, True}
}]
Attributes[Rect] = {Listable, NumericFunction}

(* Engineering sinc function *)
ESinc[x_] := Sinc[Pi x]
Attributes[ESinc] = {Listable, NumericFunction}

(* The multivariate Legendre transformation *)
LegendreTransform[f_, v_, p_] := Module[
    {
        vel = Flatten[{v}],
        mom = Flatten[{p}],
        h
    },
    First[
        h /. Quiet[
            Solve[
                h == vel . Grad[f, vel] - f && mom == Grad[f, vel],
                Append[vel, h]
            ],
            {Solve::incnst, Solve::ifun}
        ]
    ]
]

(* Other miscellaneous things *)

(* Make an augmented matrix *)
AugmentedMatrix[mat_, vec_] := Transpose@Append[Transpose[mat], vec]

(* Display the variable name when setting a value *)
ESet[var_, value_] := (
    Echo[ToString[HoldForm[var]] ~~ " is " ~~ ToString[value]];
    var = value;
)
Attributes[ESet] = {HoldFirst, SequenceHold};

(* Complex to polar form *)
ToPolar[z_] :=
 With[{n = Simplify@Abs[z], a = Simplify@Arg[z]}, Defer[n E^(I a)]]
Attributes[ESinc] = {Listable, NumericFunction};

(* Twin y plot (based on https://reference.wolfram.com/language/howto/GeneratePlotsWithTwoVerticalScales.html) *)
TwoAxisPlot[{f_, g_}, options___] :=
 Module[{fgraph, ggraph, frange, grange, fticks,
   gticks}, {fgraph, ggraph} =
   MapIndexed[
    Plot[#, options, Axes -> True,
      PlotStyle -> ColorData[1][#2[[1]]]] &, {f, g}]; {frange,
    grange} = (PlotRange /.
        AbsoluteOptions[#, PlotRange])[[2]] & /@ {fgraph, ggraph};
  fticks = N@FindDivisions[frange, 5];
  gticks =
   Quiet@Transpose@{fticks,
      ToString[NumberForm[#, 2], StandardForm] & /@
       Rescale[fticks, frange, grange]};
  Show[fgraph,
   ggraph /. Graphics[graph_, s___] :>
     Graphics[
      GeometricTransformation[graph,
       RescalingTransform[{{0, 1}, grange}, {{0, 1}, frange}]], s],
   Axes -> False, Frame -> True,
   FrameStyle -> {ColorData[1] /@ {1, 2}, {Automatic, Automatic}},
   FrameTicks -> {{fticks, gticks}, {Automatic, Automatic}}]]

(* FT to Time Domain Numeric Plot
 * This function takes a function in the frequency domain and plots the time-domain result.
 * It does not attempt to evaluate the inverse transform but instead uses numerical integration.
 * Therefore, it is useful for complicated functions but will be much slower for simple functions with a known inverse FT.
 *)
PlotFourierTransformed[ft_, {tmin_, tmax_}, tscale_:1, scalarFn_:Abs, plotOptions___] := Block[
    {ftcore = Simplify[1/Abs[tscale] ft[\[Omega]/tscale]]},
    Plot[scalarFn[1/(2\[Pi]) NIntegrate[Evaluate[ftcore Exp[I \[Omega] t]], {\[Omega], -Infinity, Infinity}]], {t, tmin, tmax}, plotOptions]
]
