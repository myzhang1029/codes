(** User Mathematica initialization file **)

(* Always prefer metric units *)
$UnitSystem = "Metric"

(* Statistics Tools *)

(* Derivative of `Abs` *)
Abs'[x_] := Sign[x] /; x != 0

(* Normal CDF *)
Phi[x_] := Erfc[-(x/Sqrt[2])]/2

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

(* Decibel conversion *)
dBu[dbu_] := First@SolveValues[
    20 Log[10, x/Quantity[Sqrt[3/5], "Volts"]] == dbu,
    x,
    Reals
]
dBV[dbv_] := First@SolveValues[
    20 Log[10, x/Quantity[1, "Volts"]] == dbv,
    x,
    Reals
]

(* Fourier analysis tools *)
IDFT[(\[Omega]_)?(NumberQ[#] || QuantityQ[#] &), times_, vals_] :=
 Mean[vals E^(-I \[Omega] times)]
VFT2dB[x_] := 20 Log10@Abs[x]

(* Make an augmented matrix *)
AugmentedMatrix[mat_, vec_] := Transpose@Append[Transpose[mat], vec]

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
