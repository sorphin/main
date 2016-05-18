/*
 * Copyright (c) 2015, Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include "statistics_test.h"

#define ERR_INVALIDPARAMTER_VALUE 123456789
#define ERR_DEVIDED_BY_ZERO_VALUE 987654321

const float udr_len1_0_1[] = { 0.874315 };
const float udr_len1_negpoint5_pospoint5[] = { -0.106264 };
const float udr_len1_0_1000[] = { 936.962280 };
const float udr_len1_neg500_pos500[] = { -63.104279 };
const float udr_len128_0_1[] =
{ 0.162522, 0.309821, 0.681109, 0.934055, 0.947389, 0.599083, 0.948874,
  0.404018,
  0.041032, 0.293826, 0.031939, 0.864513, 0.432546, 0.092809, 0.137792,
  0.241971, 0.223009, 0.867740, 0.764177, 0.344714, 0.384780, 0.594865,
  0.535121, 0.333556, 0.854683, 0.265644, 0.933940, 0.389834, 0.683065,
  0.275010, 0.027958, 0.940613, 0.533970, 0.671173, 0.607535, 0.750873,
  0.981322, 0.727670, 0.857321, 0.991843, 0.759460, 0.146041, 0.326305,
  0.028786, 0.694551, 0.958757, 0.729047, 0.736754, 0.174611, 0.355410,
  0.574649, 0.459945, 0.833725, 0.815444, 0.323952, 0.461684, 0.673959,
  0.595226, 0.134387, 0.019492, 0.125108, 0.223276, 0.449137, 0.534532,
  0.990390, 0.717347, 0.980057, 0.053738, 0.636874, 0.960382, 0.269923,
  0.949407, 0.902234, 0.194648, 0.733952, 0.174861, 0.105141, 0.314113,
  0.348766, 0.398962, 0.283917, 0.313912, 0.718280, 0.944647, 0.087789,
  0.279534, 0.596159, 0.828435, 0.782153, 0.557239, 0.036276, 0.669372,
  0.848988, 0.065465, 0.360766, 0.258051, 0.432597, 0.306071, 0.966589,
  0.129933, 0.217436, 0.893394, 0.621706, 0.398162, 0.356089, 0.646573,
  0.733091, 0.731701, 0.958176, 0.045954, 0.424361, 0.009017, 0.703805,
  0.780892, 0.564840, 0.023295, 0.007564, 0.988969, 0.201647, 0.823338,
  0.360950, 0.461253, 0.110045, 0.781262, 0.453444, 0.297102, 0.358400,
  0.482436 };
const float udr_len128_negpoint5_pospoint5[] =
{ -0.068775, 0.198823, 0.175066, -0.493129, -0.421010, -0.039373, 0.277536,
  0.316764, 0.131388, -0.135124, 0.387498, -0.249073, -0.433881, 0.227151,
  0.266798, 0.398326, 0.266825, 0.446883, 0.035733, 0.459781, 0.478188,
  0.022100, 0.345360, 0.398027, 0.431229, -0.042360, 0.259162, 0.438796,
  0.310701, 0.430392, -0.053043, 0.333886, 0.487781, -0.130361, -0.329233,
  0.323165, 0.087057, 0.461612, -0.010871, -0.246477, 0.178224, 0.414492,
  0.117075, -0.177501, -0.009488, -0.092545, -0.416060, 0.091960, 0.379016,
  0.053308, -0.200529, 0.397008, 0.294306, 0.275812, 0.098968, 0.057221,
  -0.200292, 0.047657, 0.491830, -0.016587, 0.184774, -0.019983, 0.246521,
  -0.288578, -0.251513, -0.402161, 0.208670, 0.355746, 0.289034, 0.242843,
  -0.395478, 0.020875, 0.346632, 0.343613, -0.122253, -0.227905, -0.374696,
  0.191353, 0.055131, -0.491526, -0.083968, -0.060882, 0.284361, 0.329997,
  0.089606, -0.357926, 0.093313, 0.226193, -0.071620, -0.289208, -0.234616,
  0.493184, -0.019244, 0.327750, 0.103238, 0.317842, 0.455547, 0.150378,
  0.127647, 0.146563, -0.437867, 0.438197, 0.398716, 0.194859, -0.189594,
  -0.156454, -0.423771, 0.019837, 0.108777, 0.227160, -0.499926, -0.330234,
  -0.036709, -0.138148, 0.452066, 0.432193, 0.458069, -0.293462, -0.340299,
  0.071761, 0.092919, 0.198611, -0.194187, -0.106149, -0.163114, -0.371007,
  -0.413064, 0.048943 };
const float udr_len128_0_1000[] =
{ 317.733246, 992.747620, 723.565308, 572.101440, 717.227173, 976.559204,
  426.990570, 913.013733, 897.311279, 835.228577, 47.958172, 359.280548,
  295.753906, 629.125610, 136.183792, 374.045776, 864.781677, 250.273590,
  29.505579, 999.448975, 605.142639, 244.195114, 438.195251, 735.093567,
  557.989136, 438.339386, 603.541809, 180.998810, 808.786133, 819.058411,
  487.235718, 755.050415, 404.372528, 725.676270, 154.008575, 509.073242,
  668.035583, 152.575653, 62.696018, 337.114868, 445.943970, 757.648438,
  130.533569, 901.511841, 220.338242, 378.550751, 422.755890, 972.686157,
  177.016052, 882.276550, 551.257385, 321.491882, 899.845581, 542.111694,
  462.889252, 807.974915, 360.818237, 971.002991, 555.517334, 769.819702,
  680.121033, 205.199554, 834.622925, 865.467041, 41.520782, 614.192993,
  273.280243, 746.706909, 487.871948, 792.375488, 524.462891, 597.511536,
  220.030197, 364.019257, 297.242035, 851.616943, 973.689636, 623.621582,
  892.279236, 980.902710, 646.979065, 798.818542, 404.874451, 482.364441,
  63.892357, 802.928528, 351.071014, 849.272400, 991.807251, 484.323944,
  356.364594, 54.371086, 914.077026, 704.968750, 438.083618, 968.109009,
  723.820862, 656.803284, 514.771606, 877.056213, 60.174160, 471.477661,
  311.016571, 194.693069, 151.049545, 901.420410, 70.658295, 958.387756,
  699.889709, 12.009664, 4.880399, 853.291382, 467.287781, 836.718689,
  220.624802, 805.960449, 248.579437, 998.272583, 937.897644, 553.956360,
  912.697449, 635.489929, 780.095764, 567.218201, 191.761673, 906.248352,
  514.856384, 998.976318 };
const float udr_len128_neg500_pos500[] =
{ -122.671326, -293.888000, 229.946716, -186.564331, -382.419067, 388.605591,
  193.122925, 439.900146, -82.832703, 315.770874, 304.842712, -139.595703,
  413.814148, -211.684174, -334.998138, -44.137482, -257.182678,
  -498.077942, 115.342834, 161.227905, 160.336426, 180.475159, 350.649658,
  -462.685547, 180.775208, 71.137634, 190.214783, 395.566467, -233.137695,
  -431.445374, -186.398529, 289.179504, -1.660156, -62.967285, -331.594299,
  -235.437347, 183.881592, 316.637390, 205.520996, 76.538940, 494.062500,
  -38.806732, -6.646759, -226.040314, 155.102905, -92.942902, -200.036865,
  105.435547, 291.173279, 470.893555, 273.383850, 99.981750, -50.931366,
  -346.527222, -288.766998, 200.349060, 456.182190, 135.468689, 391.271179,
  -283.714966, 183.543701, -184.941376, -474.613403, 320.249023,
  -67.331299, -411.293396, 435.083984, -109.223175, 87.931641, -260.164063,
  153.565002, -133.467529, -18.750061, -98.314423, -166.202454,
  -232.414642, 267.842957, -259.618073, 461.277161, 423.551270,
  -225.753113, -276.721008, -247.876434, -253.006577, -260.423615,
  -71.749023, 325.925903, 117.186218, -306.125610, 308.081421, 485.634216,
  -351.397522, 353.030823, -261.398041, -83.059448, -177.600098,
  -65.961365, -172.576263, -401.031219, 171.174377, -450.641296,
  -299.004089, -0.863068, -224.580688, -80.396088, 96.992920, -450.855103,
  386.679382, 358.299194, -414.938446, -322.583221, -486.601471, 89.985413,
  207.862000, 258.922607, -286.028717, 466.167297, -15.611023, -437.531891,
  -321.540802, -161.736603, 380.482178, -457.279449, -144.592590,
  140.739319, 327.883728, -455.376984, 290.825623 };
const float constdata_len128[] =
{ 1.100000, 1.100000, 1.100000, 1.100000, 1.100000, 1.100000, 1.100000,
  1.100000,
  1.100000, 1.100000, 1.100000, 1.100000, 1.100000, 1.100000, 1.100000,
  1.100000, 1.100000, 1.100000, 1.100000, 1.100000, 1.100000, 1.100000,
  1.100000, 1.100000, 1.100000, 1.100000, 1.100000, 1.100000, 1.100000,
  1.100000, 1.100000, 1.100000, 1.100000, 1.100000, 1.100000, 1.100000,
  1.100000, 1.100000, 1.100000, 1.100000, 1.100000, 1.100000, 1.100000,
  1.100000, 1.100000, 1.100000, 1.100000, 1.100000, 1.100000, 1.100000,
  1.100000, 1.100000, 1.100000, 1.100000, 1.100000, 1.100000, 1.100000,
  1.100000, 1.100000, 1.100000, 1.100000, 1.100000, 1.100000, 1.100000,
  1.100000, 1.100000, 1.100000, 1.100000, 1.100000, 1.100000, 1.100000,
  1.100000, 1.100000, 1.100000, 1.100000, 1.100000, 1.100000, 1.100000,
  1.100000, 1.100000, 1.100000, 1.100000, 1.100000, 1.100000, 1.100000,
  1.100000, 1.100000, 1.100000, 1.100000, 1.100000, 1.100000, 1.100000,
  1.100000, 1.100000, 1.100000, 1.100000, 1.100000, 1.100000, 1.100000,
  1.100000, 1.100000, 1.100000, 1.100000, 1.100000, 1.100000, 1.100000,
  1.100000, 1.100000, 1.100000, 1.100000, 1.100000, 1.100000, 1.100000,
  1.100000, 1.100000, 1.100000, 1.100000, 1.100000, 1.100000, 1.100000,
  1.100000, 1.100000, 1.100000, 1.100000, 1.100000, 1.100000, 1.100000,
  1.100000 };
const float exceptiondata_len0[] = { 0 };

float *pTestData[] =
{ (float *)udr_len1_0_1, (float *)udr_len1_negpoint5_pospoint5,
  (float *)udr_len1_0_1000, (float *)udr_len1_neg500_pos500,
  (float *)udr_len128_0_1, (float *)udr_len128_negpoint5_pospoint5,
  (float *)udr_len128_0_1000, (float *)udr_len128_neg500_pos500,
  (float *)constdata_len128, NULL, (float *)exceptiondata_len0 };
const int DataLen[] = { 1, 1, 1, 1, 128, 128, 128, 128, 128, 10, 0 };
const float sum[] =
{ 0.874315, -0.106264, 936.962280, -63.104279, 64.429848, 8.514574,
  71795.085938,
  -1279.257324, 140.799942, 123456789.000000, 123456789.000000 };
const float mean[] =
{ 0.874315, -0.106264, 936.962280, -63.104279, 0.503358, 0.066520, 560.899109,
  -9.994198, 1.100000, 123456789.000000, 123456789.000000 };
const float std0[] =
{ 0.000000, 0.000000, 0.000000, 0.000000, 0.302179, 0.281016, 292.329010,
  284.327362, 0.000000, 123456789.000000, 123456789.000000 };
const float std1[] =
{ 0.000000, 0.000000, 0.000000, 0.000000, 0.300997, 0.279916, 291.184875,
  283.214539, 0.000000, 123456789.000000, 123456789.000000 };
const float kurtosis[] =
{ 987654321.000000, 987654321.000000, 987654321.000000, 987654321.000000,
  1.757049, 2.000827, 1.863613, 1.789174, 987654321.000000,
  123456789.000000, 123456789.000000 };
const float skewness[] =
{ 987654321.000000, 987654321.000000, 987654321.000000, 987654321.000000,
  0.024939, -0.305173, -0.206727, 0.082998, 987654321.000000,
  123456789.000000, 123456789.000000 };
const float negentropy[] =
{ 987654321.000000, 987654321.000000, 987654321.000000, 987654321.000000,
  0.064317, 0.083406, 2170820100096.000000, 296244379648.000000,
  987654321.000000, 123456789.000000, 123456789.000000 };
const float min[] =
{ 0.874315, -0.106264, 936.962280, -63.104279, 0.007564, -0.499926, 4.880399,
  -498.077942, 1.100000, 123456789.000000, 123456789.000000 };
const float max[] =
{ 0.874315, -0.106264, 936.962280, -63.104279, 0.991843, 0.493184, 999.448975,
  494.062500, 1.100000, 123456789.000000, 123456789.000000 };
const float kmoment1[] =
{ 0.874315, -0.106264, 936.962280, -63.104279, 0.503358, 0.066520, 560.899109,
  -9.994198, 1.100000, 123456789.000000, 123456789.000000 };
const float kcmoment1[] =
{ 0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000,
  0.000000,
  0.000000, 123456789.000000, 123456789.000000 };
const float kmoment2[] =
{ 0.764427, 0.011292, 877898.312500, 3982.149902, 0.343968, 0.082778,
  399396.468750, 80310.367188, 1.210001, 123456789.000000,
  123456789.000000 };
const float kcmoment2[] =
{ 0.000000, 0.000000, 0.000000, 0.000000, 0.090599, 0.078353, 84788.625000,
  80210.468750, 0.000000, 123456789.000000, 123456789.000000 };
const float kmoment3[] =
{ 0.668350, -0.001200, 822557632.000000, -251290.703125, 0.265027, 0.009237,
  314032928.000000, -520462.750000, 1.330999, 123456789.000000,
  123456789.000000 };
const float kcmoment3[] =
{ 0.000000, 0.000000, 0.000000, 0.000000, 0.000680, -0.006693, -5103904.500000,
  1885452.875000, 0.000000, 123456789.000000, 123456789.000000 };
const float kmoment4[] =
{ 0.584349, 0.000128, 770705457152.000000, 15857518.000000, 0.217717, 0.012602,
  260975706112.000000, 11483753472.000000, 1.464100, 123456789.000000,
  123456789.000000 };
const float kcmoment4[] =
{ 0.000000, 0.000000, 0.000000, 0.000000, 0.014422, 0.012284,
  13397723136.000000,
  11511045120.000000, 0.000000, 123456789.000000, 123456789.000000 };

typedef int (*pFun1)(float *, int, int, float);
#define FUN1_NUM 7
pFun1 apFun1[FUN1_NUM] =
{ Sum_test, Mean_test, Kurtosis_test, Skewness_test, NegEntropy_test,
  Max_Array_test, Min_Array_test };
float *apfRightRes1[] =
{ (float *)sum, (float *)mean, (float *)kurtosis, (float *)skewness,
  (float *)negentropy, (float *)max, (float *)min };
char *apstrFun1Name[] =
{ "Sum", "Mean", "Kurtosis", "Skewness", "NegEntropy", "Max_Array", "Min_Array" };

typedef int (*pFun2)(float *, int, int, int, float);
#define FUN2_NUM 10
pFun2 apFun2[FUN2_NUM] =
{ Var_test, Var_test, K_Center_Moment_test, K_Center_Moment_test,
  K_Center_Moment_test, K_Center_Moment_test, K_Moment_test, K_Moment_test,
  K_Moment_test, K_Moment_test };
float *apfRightRes2[] =
{ (float *)std0, (float *)std1, (float *)kcmoment1, (float *)kcmoment2,
  (float *)kcmoment3, (float *)kcmoment4, (float *)kmoment1,
  (float *)kmoment2, (float *)kmoment3, (float *)kmoment4 };
char *apstrFun2Name[] =
{ "Var0", "Var1", "K_Center_Moment1", "K_Center_Moment2", "K_Center_Moment3",
  "K_Center_Moment4", "K_Moment1", "K_Moment2", "K_Moment3", "K_Moment4" };
int anFlag[] = { 0, 1, 1, 2, 3, 4, 1, 2, 3, 4 };