/* This is a 5-point algorithm contributed to OpenCV by the author, Bo Li.
It implements the 5-point algorithm solver from Nister's paper:
Nister, An efficient solution to the five-point relative pose problem, PAMI, 2004.
*/

/*  Copyright (c) 2013, Bo Li (prclibo@gmail.com), ETH Zurich
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
* Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.
* Neither the name of the copyright holder nor the
names of its contributors may be used to endorse or promote products
derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


// MF: had to remove
//#include "precomp.hpp"
// MF: added for PointSetRegistrator class (was in precom.hpp)
//#include "myPointSetRegistrator.h"

// MF: for most things
#include <opencv2/core/core.hpp>
// MF: for triangulatePoints
#include <opencv2/calib3d/calib3d.hpp>


namespace cv
{

	class EMEstimatorCallback
	{
	public:
		int modelPoints;
		bool checkPartialSubsets;
		double threshold;
		double confidence;
		int maxIters;

	public:
		/*EMEstimatorCallback()
		{
			checkPartialSubsets = true;
		}*/

		bool checkSubset(InputArray, InputArray, int)
		{
			return true;
		}

		void DoTheThing(int nrPoints, double threshold, double prob, const Mat& points1, const Mat& points2, Mat& E, OutputArray _mask)
		{
			this->modelPoints = nrPoints;
			this->threshold = threshold;
			this->confidence = prob;
			
			this->maxIters = 1000;
			this->checkPartialSubsets = true;

			run(points1, points2, E, _mask);

		}

		int findInliers(const Mat& m1, const Mat& m2, const Mat& model, Mat& err, Mat& mask, double thresh) const
		{
			computeError(m1, m2, model, err);
			mask.create(err.size(), CV_8U);

			CV_Assert(err.isContinuous() && err.type() == CV_32F && mask.isContinuous() && mask.type() == CV_8U);
			const float* errptr = err.ptr<float>();
			uchar* maskptr = mask.ptr<uchar>();
			float t = (float)(thresh*thresh);
			int i, n = (int)err.total(), nz = 0;
			for (i = 0; i < n; i++)
			{
				int f = errptr[i] <= t;
				maskptr[i] = (uchar)f;
				nz += f;
			}
			return nz;
		}

		bool getSubset(const Mat& m1, const Mat& m2,
			Mat& ms1, Mat& ms2, RNG& rng,
			int maxAttempts = 1000)
		{
			cv::AutoBuffer<int> _idx(modelPoints);
			int* idx = _idx;
			int i = 0, j, k, iters = 0;
			int esz1 = (int)m1.elemSize(), esz2 = (int)m2.elemSize();
			int d1 = m1.channels() > 1 ? m1.channels() : m1.cols;
			int d2 = m2.channels() > 1 ? m2.channels() : m2.cols;
			int count = m1.checkVector(d1), count2 = m2.checkVector(d2);
			const int *m1ptr = (const int*)m1.data, *m2ptr = (const int*)m2.data;

			ms1.create(modelPoints, 1, CV_MAKETYPE(m1.depth(), d1));
			ms2.create(modelPoints, 1, CV_MAKETYPE(m2.depth(), d2));

			int *ms1ptr = (int*)ms1.data, *ms2ptr = (int*)ms2.data;

			CV_Assert(count >= modelPoints && count == count2);
			CV_Assert((esz1 % sizeof(int)) == 0 && (esz2 % sizeof(int)) == 0);
			esz1 /= sizeof(int);
			esz2 /= sizeof(int);

			for (; iters < maxAttempts; iters++)
			{
				for (i = 0; i < modelPoints && iters < maxAttempts;)
				{
					int idx_i = 0;
					for (;;)
					{
						idx_i = idx[i] = rng.uniform(0, count);
						for (j = 0; j < i; j++)
						if (idx_i == idx[j])
							break;
						if (j == i)
							break;
					}
					for (k = 0; k < esz1; k++)
						ms1ptr[i*esz1 + k] = m1ptr[idx_i*esz1 + k];
					for (k = 0; k < esz2; k++)
						ms2ptr[i*esz2 + k] = m2ptr[idx_i*esz2 + k];
					if (checkPartialSubsets && !checkSubset(ms1, ms2, i + 1))
					{
						iters++;
						continue;
					}
					i++;
				}
				if (!checkPartialSubsets && i == modelPoints && !checkSubset(ms1, ms2, i))
					continue;
				break;
			}

			return i == modelPoints && iters < maxAttempts;
		}

		int RANSACUpdateNumIters2(double p, double ep, int modelPoints, int maxIters)
		{
			if (modelPoints <= 0)
			{
				// MF: uwaga wywali�em!
				//CV_Error(Error::StsOutOfRange, "the number of model points should be positive");
			}


			p = MAX(p, 0.);
			p = MIN(p, 1.);
			ep = MAX(ep, 0.);
			ep = MIN(ep, 1.);

			// avoid inf's & nan's
			double num = MAX(1. - p, DBL_MIN);
			double denom = 1. - std::pow(1. - ep, modelPoints);
			if (denom < DBL_MIN)
				return 0;

			num = std::log(num);
			denom = std::log(denom);

			return denom >= 0 || -num >= maxIters*(-denom) ? maxIters : cvRound(num / denom);
		}

		bool run(InputArray _m1, InputArray _m2, OutputArray _model, OutputArray _mask) 
		{
			bool result = false;
			Mat m1 = _m1.getMat(), m2 = _m2.getMat();
			Mat err, mask, model, bestModel, ms1, ms2;

			int iter, niters = MAX(maxIters, 1);
			int d1 = m1.channels() > 1 ? m1.channels() : m1.cols;
			int d2 = m2.channels() > 1 ? m2.channels() : m2.cols;
			int count = m1.checkVector(d1), count2 = m2.checkVector(d2), maxGoodCount = 0;

			RNG rng((uint64)-1);

			// MF: wywali�em
			//CV_Assert(cb);
			CV_Assert(confidence > 0 && confidence < 1);

			CV_Assert(count >= 0 && count2 == count);
			if (count < modelPoints)
				return false;

			Mat bestMask0, bestMask;

			if (_mask.needed())
			{
				_mask.create(count, 1, CV_8U, -1, true);
				bestMask0 = bestMask = _mask.getMat();
				CV_Assert((bestMask.cols == 1 || bestMask.rows == 1) && (int)bestMask.total() == count);
			}
			else
			{
				bestMask.create(count, 1, CV_8U);
				bestMask0 = bestMask;
			}

			if (count == modelPoints)
			{
				if (runKernel(m1, m2, bestModel) <= 0)
					return false;
				bestModel.copyTo(_model);
				bestMask.setTo(Scalar::all(1));
				return true;
			}

			for (iter = 0; iter < niters; iter++)
			{
				int i, goodCount, nmodels;
				if (count > modelPoints)
				{
					bool found = getSubset(m1, m2, ms1, ms2, rng);
					if (!found)
					{
						if (iter == 0)
							return false;
						break;
					}
				}

				nmodels = runKernel(ms1, ms2, model);
				if (nmodels <= 0)
					continue;
				CV_Assert(model.rows % nmodels == 0);
				Size modelSize(model.cols, model.rows / nmodels);

				for (i = 0; i < nmodels; i++)
				{
					Mat model_i = model.rowRange(i*modelSize.height, (i + 1)*modelSize.height);
					goodCount = findInliers(m1, m2, model_i, err, mask, threshold);

					if (goodCount > MAX(maxGoodCount, modelPoints - 1))
					{
						std::swap(mask, bestMask);
						model_i.copyTo(bestModel);
						maxGoodCount = goodCount;
						niters = RANSACUpdateNumIters2(confidence, (double)(count - goodCount) / count, modelPoints, niters);
					}
				}
			}

			if (maxGoodCount > 0)
			{
				if (bestMask.data != bestMask0.data)
				{
					if (bestMask.size() == bestMask0.size())
						bestMask.copyTo(bestMask0);
					else
						transpose(bestMask, bestMask0);
				}
				bestModel.copyTo(_model);
				result = true;
			}
			else
				_model.release();

			return result;
		}

		int runKernel(InputArray _m1, InputArray _m2, OutputArray _model) const
		{
			Mat q1 = _m1.getMat(), q2 = _m2.getMat();
			Mat Q1 = q1.reshape(1, (int)q1.total());
			Mat Q2 = q2.reshape(1, (int)q2.total());

			int n = Q1.rows;
			Mat Q(n, 9, CV_64F);
			Q.col(0) = Q1.col(0).mul(Q2.col(0));
			Q.col(1) = Q1.col(1).mul(Q2.col(0));
			Q.col(2) = Q2.col(0) * 1.0;
			Q.col(3) = Q1.col(0).mul(Q2.col(1));
			Q.col(4) = Q1.col(1).mul(Q2.col(1));
			Q.col(5) = Q2.col(1) * 1.0;
			Q.col(6) = Q1.col(0) * 1.0;
			Q.col(7) = Q1.col(1) * 1.0;
			Q.col(8) = 1.0;

			Mat U, W, Vt;
			SVD::compute(Q, W, U, Vt, SVD::MODIFY_A | SVD::FULL_UV);

			Mat EE = Mat(Vt.t()).colRange(5, 9) * 1.0;
			Mat A(10, 20, CV_64F);
			EE = EE.t();
			getCoeffMat((double*)EE.data, (double*)A.data);
			EE = EE.t();

			A = A.colRange(0, 10).inv() * A.colRange(10, 20);

			double b[3 * 13];
			Mat B(3, 13, CV_64F, b);
			for (int i = 0; i < 3; i++)
			{
				Mat arow1 = A.row(i * 2 + 4) * 1.0;
				Mat arow2 = A.row(i * 2 + 5) * 1.0;
				Mat row1(1, 13, CV_64F, Scalar(0.0));
				Mat row2(1, 13, CV_64F, Scalar(0.0));

				row1.colRange(1, 4) = arow1.colRange(0, 3) * 1.0;
				row1.colRange(5, 8) = arow1.colRange(3, 6) * 1.0;
				row1.colRange(9, 13) = arow1.colRange(6, 10) * 1.0;

				row2.colRange(0, 3) = arow2.colRange(0, 3) * 1.0;
				row2.colRange(4, 7) = arow2.colRange(3, 6) * 1.0;
				row2.colRange(8, 12) = arow2.colRange(6, 10) * 1.0;

				B.row(i) = row1 - row2;
			}

			double c[11];
			Mat coeffs(1, 11, CV_64F, c);
			c[10] = (b[0] * b[17] * b[34] + b[26] * b[4] * b[21] - b[26] * b[17] * b[8] - b[13] * b[4] * b[34] - b[0] * b[21] * b[30] + b[13] * b[30] * b[8]);
			c[9] = (b[26] * b[4] * b[22] + b[14] * b[30] * b[8] + b[13] * b[31] * b[8] + b[1] * b[17] * b[34] - b[13] * b[5] * b[34] + b[26] * b[5] * b[21] - b[0] * b[21] * b[31] - b[26] * b[17] * b[9] - b[1] * b[21] * b[30] + b[27] * b[4] * b[21] + b[0] * b[17] * b[35] - b[0] * b[22] * b[30] + b[13] * b[30] * b[9] + b[0] * b[18] * b[34] - b[27] * b[17] * b[8] - b[14] * b[4] * b[34] - b[13] * b[4] * b[35] - b[26] * b[18] * b[8]);
			c[8] = (b[14] * b[30] * b[9] + b[14] * b[31] * b[8] + b[13] * b[31] * b[9] - b[13] * b[4] * b[36] - b[13] * b[5] * b[35] + b[15] * b[30] * b[8] - b[13] * b[6] * b[34] + b[13] * b[30] * b[10] + b[13] * b[32] * b[8] - b[14] * b[4] * b[35] - b[14] * b[5] * b[34] + b[26] * b[4] * b[23] + b[26] * b[5] * b[22] + b[26] * b[6] * b[21] - b[26] * b[17] * b[10] - b[15] * b[4] * b[34] - b[26] * b[18] * b[9] - b[26] * b[19] * b[8] + b[27] * b[4] * b[22] + b[27] * b[5] * b[21] - b[27] * b[17] * b[9] - b[27] * b[18] * b[8] - b[1] * b[21] * b[31] - b[0] * b[23] * b[30] - b[0] * b[21] * b[32] + b[28] * b[4] * b[21] - b[28] * b[17] * b[8] + b[2] * b[17] * b[34] + b[0] * b[18] * b[35] - b[0] * b[22] * b[31] + b[0] * b[17] * b[36] + b[0] * b[19] * b[34] - b[1] * b[22] * b[30] + b[1] * b[18] * b[34] + b[1] * b[17] * b[35] - b[2] * b[21] * b[30]);
			c[7] = (b[14] * b[30] * b[10] + b[14] * b[32] * b[8] - b[3] * b[21] * b[30] + b[3] * b[17] * b[34] + b[13] * b[32] * b[9] + b[13] * b[33] * b[8] - b[13] * b[4] * b[37] - b[13] * b[5] * b[36] + b[15] * b[30] * b[9] + b[15] * b[31] * b[8] - b[16] * b[4] * b[34] - b[13] * b[6] * b[35] - b[13] * b[7] * b[34] + b[13] * b[30] * b[11] + b[13] * b[31] * b[10] + b[14] * b[31] * b[9] - b[14] * b[4] * b[36] - b[14] * b[5] * b[35] - b[14] * b[6] * b[34] + b[16] * b[30] * b[8] - b[26] * b[20] * b[8] + b[26] * b[4] * b[24] + b[26] * b[5] * b[23] + b[26] * b[6] * b[22] + b[26] * b[7] * b[21] - b[26] * b[17] * b[11] - b[15] * b[4] * b[35] - b[15] * b[5] * b[34] - b[26] * b[18] * b[10] - b[26] * b[19] * b[9] + b[27] * b[4] * b[23] + b[27] * b[5] * b[22] + b[27] * b[6] * b[21] - b[27] * b[17] * b[10] - b[27] * b[18] * b[9] - b[27] * b[19] * b[8] + b[0] * b[17] * b[37] - b[0] * b[23] * b[31] - b[0] * b[24] * b[30] - b[0] * b[21] * b[33] - b[29] * b[17] * b[8] + b[28] * b[4] * b[22] + b[28] * b[5] * b[21] - b[28] * b[17] * b[9] - b[28] * b[18] * b[8] + b[29] * b[4] * b[21] + b[1] * b[19] * b[34] - b[2] * b[21] * b[31] + b[0] * b[20] * b[34] + b[0] * b[19] * b[35] + b[0] * b[18] * b[36] - b[0] * b[22] * b[32] - b[1] * b[23] * b[30] - b[1] * b[21] * b[32] + b[1] * b[18] * b[35] - b[1] * b[22] * b[31] - b[2] * b[22] * b[30] + b[2] * b[17] * b[35] + b[1] * b[17] * b[36] + b[2] * b[18] * b[34]);
			c[6] = (-b[14] * b[6] * b[35] - b[14] * b[7] * b[34] - b[3] * b[22] * b[30] - b[3] * b[21] * b[31] + b[3] * b[17] * b[35] + b[3] * b[18] * b[34] + b[13] * b[32] * b[10] + b[13] * b[33] * b[9] - b[13] * b[4] * b[38] - b[13] * b[5] * b[37] - b[15] * b[6] * b[34] + b[15] * b[30] * b[10] + b[15] * b[32] * b[8] - b[16] * b[4] * b[35] - b[13] * b[6] * b[36] - b[13] * b[7] * b[35] + b[13] * b[31] * b[11] + b[13] * b[30] * b[12] + b[14] * b[32] * b[9] + b[14] * b[33] * b[8] - b[14] * b[4] * b[37] - b[14] * b[5] * b[36] + b[16] * b[30] * b[9] + b[16] * b[31] * b[8] - b[26] * b[20] * b[9] + b[26] * b[4] * b[25] + b[26] * b[5] * b[24] + b[26] * b[6] * b[23] + b[26] * b[7] * b[22] - b[26] * b[17] * b[12] + b[14] * b[30] * b[11] + b[14] * b[31] * b[10] + b[15] * b[31] * b[9] - b[15] * b[4] * b[36] - b[15] * b[5] * b[35] - b[26] * b[18] * b[11] - b[26] * b[19] * b[10] - b[27] * b[20] * b[8] + b[27] * b[4] * b[24] + b[27] * b[5] * b[23] + b[27] * b[6] * b[22] + b[27] * b[7] * b[21] - b[27] * b[17] * b[11] - b[27] * b[18] * b[10] - b[27] * b[19] * b[9] - b[16] * b[5] * b[34] - b[29] * b[17] * b[9] - b[29] * b[18] * b[8] + b[28] * b[4] * b[23] + b[28] * b[5] * b[22] + b[28] * b[6] * b[21] - b[28] * b[17] * b[10] - b[28] * b[18] * b[9] - b[28] * b[19] * b[8] + b[29] * b[4] * b[22] + b[29] * b[5] * b[21] - b[2] * b[23] * b[30] + b[2] * b[18] * b[35] - b[1] * b[22] * b[32] - b[2] * b[21] * b[32] + b[2] * b[19] * b[34] + b[0] * b[19] * b[36] - b[0] * b[22] * b[33] + b[0] * b[20] * b[35] - b[0] * b[23] * b[32] - b[0] * b[25] * b[30] + b[0] * b[17] * b[38] + b[0] * b[18] * b[37] - b[0] * b[24] * b[31] + b[1] * b[17] * b[37] - b[1] * b[23] * b[31] - b[1] * b[24] * b[30] - b[1] * b[21] * b[33] + b[1] * b[20] * b[34] + b[1] * b[19] * b[35] + b[1] * b[18] * b[36] + b[2] * b[17] * b[36] - b[2] * b[22] * b[31]);
			c[5] = (-b[14] * b[6] * b[36] - b[14] * b[7] * b[35] + b[14] * b[31] * b[11] - b[3] * b[23] * b[30] - b[3] * b[21] * b[32] + b[3] * b[18] * b[35] - b[3] * b[22] * b[31] + b[3] * b[17] * b[36] + b[3] * b[19] * b[34] + b[13] * b[32] * b[11] + b[13] * b[33] * b[10] - b[13] * b[5] * b[38] - b[15] * b[6] * b[35] - b[15] * b[7] * b[34] + b[15] * b[30] * b[11] + b[15] * b[31] * b[10] + b[16] * b[31] * b[9] - b[13] * b[6] * b[37] - b[13] * b[7] * b[36] + b[13] * b[31] * b[12] + b[14] * b[32] * b[10] + b[14] * b[33] * b[9] - b[14] * b[4] * b[38] - b[14] * b[5] * b[37] - b[16] * b[6] * b[34] + b[16] * b[30] * b[10] + b[16] * b[32] * b[8] - b[26] * b[20] * b[10] + b[26] * b[5] * b[25] + b[26] * b[6] * b[24] + b[26] * b[7] * b[23] + b[14] * b[30] * b[12] + b[15] * b[32] * b[9] + b[15] * b[33] * b[8] - b[15] * b[4] * b[37] - b[15] * b[5] * b[36] + b[29] * b[5] * b[22] + b[29] * b[6] * b[21] - b[26] * b[18] * b[12] - b[26] * b[19] * b[11] - b[27] * b[20] * b[9] + b[27] * b[4] * b[25] + b[27] * b[5] * b[24] + b[27] * b[6] * b[23] + b[27] * b[7] * b[22] - b[27] * b[17] * b[12] - b[27] * b[18] * b[11] - b[27] * b[19] * b[10] - b[28] * b[20] * b[8] - b[16] * b[4] * b[36] - b[16] * b[5] * b[35] - b[29] * b[17] * b[10] - b[29] * b[18] * b[9] - b[29] * b[19] * b[8] + b[28] * b[4] * b[24] + b[28] * b[5] * b[23] + b[28] * b[6] * b[22] + b[28] * b[7] * b[21] - b[28] * b[17] * b[11] - b[28] * b[18] * b[10] - b[28] * b[19] * b[9] + b[29] * b[4] * b[23] - b[2] * b[22] * b[32] - b[2] * b[21] * b[33] - b[1] * b[24] * b[31] + b[0] * b[18] * b[38] - b[0] * b[24] * b[32] + b[0] * b[19] * b[37] + b[0] * b[20] * b[36] - b[0] * b[25] * b[31] - b[0] * b[23] * b[33] + b[1] * b[19] * b[36] - b[1] * b[22] * b[33] + b[1] * b[20] * b[35] + b[2] * b[19] * b[35] - b[2] * b[24] * b[30] - b[2] * b[23] * b[31] + b[2] * b[20] * b[34] + b[2] * b[17] * b[37] - b[1] * b[25] * b[30] + b[1] * b[18] * b[37] + b[1] * b[17] * b[38] - b[1] * b[23] * b[32] + b[2] * b[18] * b[36]);
			c[4] = (-b[14] * b[6] * b[37] - b[14] * b[7] * b[36] + b[14] * b[31] * b[12] + b[3] * b[17] * b[37] - b[3] * b[23] * b[31] - b[3] * b[24] * b[30] - b[3] * b[21] * b[33] + b[3] * b[20] * b[34] + b[3] * b[19] * b[35] + b[3] * b[18] * b[36] - b[3] * b[22] * b[32] + b[13] * b[32] * b[12] + b[13] * b[33] * b[11] - b[15] * b[6] * b[36] - b[15] * b[7] * b[35] + b[15] * b[31] * b[11] + b[15] * b[30] * b[12] + b[16] * b[32] * b[9] + b[16] * b[33] * b[8] - b[13] * b[6] * b[38] - b[13] * b[7] * b[37] + b[14] * b[32] * b[11] + b[14] * b[33] * b[10] - b[14] * b[5] * b[38] - b[16] * b[6] * b[35] - b[16] * b[7] * b[34] + b[16] * b[30] * b[11] + b[16] * b[31] * b[10] - b[26] * b[19] * b[12] - b[26] * b[20] * b[11] + b[26] * b[6] * b[25] + b[26] * b[7] * b[24] + b[15] * b[32] * b[10] + b[15] * b[33] * b[9] - b[15] * b[4] * b[38] - b[15] * b[5] * b[37] + b[29] * b[5] * b[23] + b[29] * b[6] * b[22] + b[29] * b[7] * b[21] - b[27] * b[20] * b[10] + b[27] * b[5] * b[25] + b[27] * b[6] * b[24] + b[27] * b[7] * b[23] - b[27] * b[18] * b[12] - b[27] * b[19] * b[11] - b[28] * b[20] * b[9] - b[16] * b[4] * b[37] - b[16] * b[5] * b[36] + b[0] * b[19] * b[38] - b[0] * b[24] * b[33] + b[0] * b[20] * b[37] - b[29] * b[17] * b[11] - b[29] * b[18] * b[10] - b[29] * b[19] * b[9] + b[28] * b[4] * b[25] + b[28] * b[5] * b[24] + b[28] * b[6] * b[23] + b[28] * b[7] * b[22] - b[28] * b[17] * b[12] - b[28] * b[18] * b[11] - b[28] * b[19] * b[10] - b[29] * b[20] * b[8] + b[29] * b[4] * b[24] + b[2] * b[18] * b[37] - b[0] * b[25] * b[32] + b[1] * b[18] * b[38] - b[1] * b[24] * b[32] + b[1] * b[19] * b[37] + b[1] * b[20] * b[36] - b[1] * b[25] * b[31] + b[2] * b[17] * b[38] + b[2] * b[19] * b[36] - b[2] * b[24] * b[31] - b[2] * b[22] * b[33] - b[2] * b[23] * b[32] + b[2] * b[20] * b[35] - b[1] * b[23] * b[33] - b[2] * b[25] * b[30]);
			c[3] = (-b[14] * b[6] * b[38] - b[14] * b[7] * b[37] + b[3] * b[19] * b[36] - b[3] * b[22] * b[33] + b[3] * b[20] * b[35] - b[3] * b[23] * b[32] - b[3] * b[25] * b[30] + b[3] * b[17] * b[38] + b[3] * b[18] * b[37] - b[3] * b[24] * b[31] - b[15] * b[6] * b[37] - b[15] * b[7] * b[36] + b[15] * b[31] * b[12] + b[16] * b[32] * b[10] + b[16] * b[33] * b[9] + b[13] * b[33] * b[12] - b[13] * b[7] * b[38] + b[14] * b[32] * b[12] + b[14] * b[33] * b[11] - b[16] * b[6] * b[36] - b[16] * b[7] * b[35] + b[16] * b[31] * b[11] + b[16] * b[30] * b[12] + b[15] * b[32] * b[11] + b[15] * b[33] * b[10] - b[15] * b[5] * b[38] + b[29] * b[5] * b[24] + b[29] * b[6] * b[23] - b[26] * b[20] * b[12] + b[26] * b[7] * b[25] - b[27] * b[19] * b[12] - b[27] * b[20] * b[11] + b[27] * b[6] * b[25] + b[27] * b[7] * b[24] - b[28] * b[20] * b[10] - b[16] * b[4] * b[38] - b[16] * b[5] * b[37] + b[29] * b[7] * b[22] - b[29] * b[17] * b[12] - b[29] * b[18] * b[11] - b[29] * b[19] * b[10] + b[28] * b[5] * b[25] + b[28] * b[6] * b[24] + b[28] * b[7] * b[23] - b[28] * b[18] * b[12] - b[28] * b[19] * b[11] - b[29] * b[20] * b[9] + b[29] * b[4] * b[25] - b[2] * b[24] * b[32] + b[0] * b[20] * b[38] - b[0] * b[25] * b[33] + b[1] * b[19] * b[38] - b[1] * b[24] * b[33] + b[1] * b[20] * b[37] - b[2] * b[25] * b[31] + b[2] * b[20] * b[36] - b[1] * b[25] * b[32] + b[2] * b[19] * b[37] + b[2] * b[18] * b[38] - b[2] * b[23] * b[33]);
			c[2] = (b[3] * b[18] * b[38] - b[3] * b[24] * b[32] + b[3] * b[19] * b[37] + b[3] * b[20] * b[36] - b[3] * b[25] * b[31] - b[3] * b[23] * b[33] - b[15] * b[6] * b[38] - b[15] * b[7] * b[37] + b[16] * b[32] * b[11] + b[16] * b[33] * b[10] - b[16] * b[5] * b[38] - b[16] * b[6] * b[37] - b[16] * b[7] * b[36] + b[16] * b[31] * b[12] + b[14] * b[33] * b[12] - b[14] * b[7] * b[38] + b[15] * b[32] * b[12] + b[15] * b[33] * b[11] + b[29] * b[5] * b[25] + b[29] * b[6] * b[24] - b[27] * b[20] * b[12] + b[27] * b[7] * b[25] - b[28] * b[19] * b[12] - b[28] * b[20] * b[11] + b[29] * b[7] * b[23] - b[29] * b[18] * b[12] - b[29] * b[19] * b[11] + b[28] * b[6] * b[25] + b[28] * b[7] * b[24] - b[29] * b[20] * b[10] + b[2] * b[19] * b[38] - b[1] * b[25] * b[33] + b[2] * b[20] * b[37] - b[2] * b[24] * b[33] - b[2] * b[25] * b[32] + b[1] * b[20] * b[38]);
			c[1] = (b[29] * b[7] * b[24] - b[29] * b[20] * b[11] + b[2] * b[20] * b[38] - b[2] * b[25] * b[33] - b[28] * b[20] * b[12] + b[28] * b[7] * b[25] - b[29] * b[19] * b[12] - b[3] * b[24] * b[33] + b[15] * b[33] * b[12] + b[3] * b[19] * b[38] - b[16] * b[6] * b[38] + b[3] * b[20] * b[37] + b[16] * b[32] * b[12] + b[29] * b[6] * b[25] - b[16] * b[7] * b[37] - b[3] * b[25] * b[32] - b[15] * b[7] * b[38] + b[16] * b[33] * b[11]);
			c[0] = -b[29] * b[20] * b[12] + b[29] * b[7] * b[25] + b[16] * b[33] * b[12] - b[16] * b[7] * b[38] + b[3] * b[20] * b[38] - b[3] * b[25] * b[33];

			std::vector<Complex<double> > roots;
			solvePoly(coeffs, roots);

			std::vector<double> xs, ys, zs;
			int count = 0;

			Mat ematrix(10 * 3, 3, CV_64F);
			double* e = ematrix.ptr<double>();
			for (size_t i = 0; i < roots.size(); i++)
			{
				if (fabs(roots[i].im) > 1e-10) continue;
				double z1 = roots[i].re;
				double z2 = z1 * z1;
				double z3 = z2 * z1;
				double z4 = z3 * z1;

				double bz[3][3];
				for (int j = 0; j < 3; j++)
				{
					const double * br = b + j * 13;
					bz[j][0] = br[0] * z3 + br[1] * z2 + br[2] * z1 + br[3];
					bz[j][1] = br[4] * z3 + br[5] * z2 + br[6] * z1 + br[7];
					bz[j][2] = br[8] * z4 + br[9] * z3 + br[10] * z2 + br[11] * z1 + br[12];
				}

				Mat Bz(3, 3, CV_64F, bz);
				cv::Mat xy1;
				SVD::solveZ(Bz, xy1);

				if (fabs(xy1.at<double>(2)) < 1e-10) continue;
				xs.push_back(xy1.at<double>(0) / xy1.at<double>(2));
				ys.push_back(xy1.at<double>(1) / xy1.at<double>(2));
				zs.push_back(z1);

				cv::Mat Evec = EE.col(0) * xs.back() + EE.col(1) * ys.back() + EE.col(2) * zs.back() + EE.col(3);
				Evec /= norm(Evec);

				memcpy(e + count * 9, Evec.data, 9 * sizeof(double));
				count++;
			}

			ematrix.rowRange(0, count * 3).copyTo(_model);
			return count;
		}

	protected:
		void getCoeffMat(double *e, double *A) const
		{
			double ep2[36], ep3[36];
			for (int i = 0; i < 36; i++)
			{
				ep2[i] = e[i] * e[i];
				ep3[i] = ep2[i] * e[i];
			}

			A[0] = e[33] * e[28] * e[32] - e[33] * e[31] * e[29] + e[30] * e[34] * e[29] - e[30] * e[28] * e[35] - e[27] * e[32] * e[34] + e[27] * e[31] * e[35];
			A[146] = .5000000000*e[6] * ep2[8] - .5000000000*e[6] * ep2[5] + .5000000000*ep3[6] + .5000000000*e[6] * ep2[7] - .5000000000*e[6] * ep2[4] + e[0] * e[2] * e[8] + e[3] * e[4] * e[7] + e[3] * e[5] * e[8] + e[0] * e[1] * e[7] - .5000000000*e[6] * ep2[1] - .5000000000*e[6] * ep2[2] + .5000000000*ep2[0] * e[6] + .5000000000*ep2[3] * e[6];
			A[1] = e[30] * e[34] * e[2] + e[33] * e[1] * e[32] - e[3] * e[28] * e[35] + e[0] * e[31] * e[35] + e[3] * e[34] * e[29] - e[30] * e[1] * e[35] + e[27] * e[31] * e[8] - e[27] * e[32] * e[7] - e[30] * e[28] * e[8] - e[33] * e[31] * e[2] - e[0] * e[32] * e[34] + e[6] * e[28] * e[32] - e[33] * e[4] * e[29] + e[33] * e[28] * e[5] + e[30] * e[7] * e[29] + e[27] * e[4] * e[35] - e[27] * e[5] * e[34] - e[6] * e[31] * e[29];
			A[147] = e[9] * e[27] * e[15] + e[9] * e[29] * e[17] + e[9] * e[11] * e[35] + e[9] * e[28] * e[16] + e[9] * e[10] * e[34] + e[27] * e[11] * e[17] + e[27] * e[10] * e[16] + e[12] * e[30] * e[15] + e[12] * e[32] * e[17] + e[12] * e[14] * e[35] + e[12] * e[31] * e[16] + e[12] * e[13] * e[34] + e[30] * e[14] * e[17] + e[30] * e[13] * e[16] + e[15] * e[35] * e[17] + e[15] * e[34] * e[16] - 1.*e[15] * e[28] * e[10] - 1.*e[15] * e[31] * e[13] - 1.*e[15] * e[32] * e[14] - 1.*e[15] * e[29] * e[11] + .5000000000*ep2[9] * e[33] + .5000000000*e[33] * ep2[16] - .5000000000*e[33] * ep2[11] + .5000000000*e[33] * ep2[12] + 1.500000000*e[33] * ep2[15] + .5000000000*e[33] * ep2[17] - .5000000000*e[33] * ep2[10] - .5000000000*e[33] * ep2[14] - .5000000000*e[33] * ep2[13];
			A[2] = -e[33] * e[22] * e[29] - e[33] * e[31] * e[20] - e[27] * e[32] * e[25] + e[27] * e[22] * e[35] - e[27] * e[23] * e[34] + e[27] * e[31] * e[26] + e[33] * e[28] * e[23] - e[21] * e[28] * e[35] + e[30] * e[25] * e[29] + e[24] * e[28] * e[32] - e[24] * e[31] * e[29] + e[18] * e[31] * e[35] - e[30] * e[28] * e[26] - e[30] * e[19] * e[35] + e[21] * e[34] * e[29] + e[33] * e[19] * e[32] - e[18] * e[32] * e[34] + e[30] * e[34] * e[20];
			A[144] = e[18] * e[2] * e[17] + e[3] * e[21] * e[15] + e[3] * e[12] * e[24] + e[3] * e[23] * e[17] + e[3] * e[14] * e[26] + e[3] * e[22] * e[16] + e[3] * e[13] * e[25] + 3.*e[6] * e[24] * e[15] + e[6] * e[26] * e[17] + e[6] * e[25] * e[16] + e[0] * e[20] * e[17] + e[0] * e[11] * e[26] + e[0] * e[19] * e[16] + e[0] * e[10] * e[25] + e[15] * e[26] * e[8] - 1.*e[15] * e[20] * e[2] - 1.*e[15] * e[19] * e[1] - 1.*e[15] * e[22] * e[4] + e[15] * e[25] * e[7] - 1.*e[15] * e[23] * e[5] + e[12] * e[21] * e[6] + e[12] * e[22] * e[7] + e[12] * e[4] * e[25] + e[12] * e[23] * e[8] + e[12] * e[5] * e[26] - 1.*e[24] * e[11] * e[2] - 1.*e[24] * e[10] * e[1] - 1.*e[24] * e[13] * e[4] + e[24] * e[16] * e[7] - 1.*e[24] * e[14] * e[5] + e[24] * e[17] * e[8] + e[21] * e[13] * e[7] + e[21] * e[4] * e[16] + e[21] * e[14] * e[8] + e[21] * e[5] * e[17] - 1.*e[6] * e[23] * e[14] - 1.*e[6] * e[20] * e[11] - 1.*e[6] * e[19] * e[10] - 1.*e[6] * e[22] * e[13] + e[9] * e[18] * e[6] + e[9] * e[0] * e[24] + e[9] * e[19] * e[7] + e[9] * e[1] * e[25] + e[9] * e[20] * e[8] + e[9] * e[2] * e[26] + e[18] * e[0] * e[15] + e[18] * e[10] * e[7] + e[18] * e[1] * e[16] + e[18] * e[11] * e[8];
			A[3] = e[33] * e[10] * e[32] + e[33] * e[28] * e[14] - e[33] * e[13] * e[29] - e[33] * e[31] * e[11] + e[9] * e[31] * e[35] - e[9] * e[32] * e[34] + e[27] * e[13] * e[35] - e[27] * e[32] * e[16] + e[27] * e[31] * e[17] - e[27] * e[14] * e[34] + e[12] * e[34] * e[29] - e[12] * e[28] * e[35] + e[30] * e[34] * e[11] + e[30] * e[16] * e[29] - e[30] * e[10] * e[35] - e[30] * e[28] * e[17] + e[15] * e[28] * e[32] - e[15] * e[31] * e[29];
			A[145] = e[0] * e[27] * e[6] + e[0] * e[28] * e[7] + e[0] * e[1] * e[34] + e[0] * e[29] * e[8] + e[0] * e[2] * e[35] + e[6] * e[34] * e[7] - 1.*e[6] * e[32] * e[5] + e[6] * e[30] * e[3] + e[6] * e[35] * e[8] - 1.*e[6] * e[29] * e[2] - 1.*e[6] * e[28] * e[1] - 1.*e[6] * e[31] * e[4] + e[27] * e[1] * e[7] + e[27] * e[2] * e[8] + e[3] * e[31] * e[7] + e[3] * e[4] * e[34] + e[3] * e[32] * e[8] + e[3] * e[5] * e[35] + e[30] * e[4] * e[7] + e[30] * e[5] * e[8] + .5000000000*ep2[0] * e[33] + 1.500000000*e[33] * ep2[6] - .5000000000*e[33] * ep2[4] - .5000000000*e[33] * ep2[5] - .5000000000*e[33] * ep2[1] + .5000000000*e[33] * ep2[7] + .5000000000*e[33] * ep2[3] - .5000000000*e[33] * ep2[2] + .5000000000*e[33] * ep2[8];
			A[4] = -e[0] * e[23] * e[16] + e[9] * e[4] * e[26] + e[9] * e[22] * e[8] - e[9] * e[5] * e[25] - e[9] * e[23] * e[7] + e[18] * e[4] * e[17] + e[18] * e[13] * e[8] - e[18] * e[5] * e[16] - e[18] * e[14] * e[7] + e[3] * e[16] * e[20] + e[3] * e[25] * e[11] - e[3] * e[10] * e[26] - e[3] * e[19] * e[17] + e[12] * e[7] * e[20] + e[12] * e[25] * e[2] - e[12] * e[1] * e[26] - e[12] * e[19] * e[8] + e[21] * e[7] * e[11] + e[21] * e[16] * e[2] - e[21] * e[1] * e[17] - e[21] * e[10] * e[8] + e[6] * e[10] * e[23] + e[6] * e[19] * e[14] - e[6] * e[13] * e[20] - e[6] * e[22] * e[11] + e[15] * e[1] * e[23] + e[15] * e[19] * e[5] - e[15] * e[4] * e[20] - e[15] * e[22] * e[2] + e[24] * e[1] * e[14] + e[24] * e[10] * e[5] - e[24] * e[4] * e[11] - e[24] * e[13] * e[2] + e[0] * e[13] * e[26] + e[0] * e[22] * e[17] - e[0] * e[14] * e[25];
			A[150] = e[18] * e[19] * e[25] + .5000000000*ep3[24] - .5000000000*e[24] * ep2[23] + e[18] * e[20] * e[26] + e[21] * e[22] * e[25] + e[21] * e[23] * e[26] - .5000000000*e[24] * ep2[19] + .5000000000*ep2[21] * e[24] + .5000000000*e[24] * ep2[26] - .5000000000*e[24] * ep2[20] + .5000000000*ep2[18] * e[24] - .5000000000*e[24] * ep2[22] + .5000000000*e[24] * ep2[25];
			A[5] = -e[3] * e[1] * e[35] - e[0] * e[32] * e[7] + e[27] * e[4] * e[8] + e[33] * e[1] * e[5] - e[33] * e[4] * e[2] + e[0] * e[4] * e[35] + e[3] * e[34] * e[2] - e[30] * e[1] * e[8] + e[30] * e[7] * e[2] - e[6] * e[4] * e[29] + e[3] * e[7] * e[29] + e[6] * e[1] * e[32] - e[0] * e[5] * e[34] - e[3] * e[28] * e[8] + e[0] * e[31] * e[8] + e[6] * e[28] * e[5] - e[6] * e[31] * e[2] - e[27] * e[5] * e[7];
			A[151] = e[33] * e[16] * e[7] - 1.*e[33] * e[14] * e[5] + e[33] * e[17] * e[8] + e[30] * e[13] * e[7] + e[30] * e[4] * e[16] + e[30] * e[14] * e[8] + e[30] * e[5] * e[17] + e[6] * e[27] * e[9] - 1.*e[6] * e[28] * e[10] - 1.*e[6] * e[31] * e[13] - 1.*e[6] * e[32] * e[14] - 1.*e[6] * e[29] * e[11] + e[9] * e[28] * e[7] + e[9] * e[1] * e[34] + e[9] * e[29] * e[8] + e[9] * e[2] * e[35] + e[27] * e[10] * e[7] + e[27] * e[1] * e[16] + e[27] * e[11] * e[8] + e[27] * e[2] * e[17] + e[3] * e[30] * e[15] + e[3] * e[12] * e[33] + e[3] * e[32] * e[17] + e[3] * e[14] * e[35] + e[3] * e[31] * e[16] + e[3] * e[13] * e[34] + 3.*e[6] * e[33] * e[15] + e[6] * e[35] * e[17] + e[6] * e[34] * e[16] + e[0] * e[27] * e[15] + e[0] * e[9] * e[33] + e[0] * e[29] * e[17] + e[0] * e[11] * e[35] + e[0] * e[28] * e[16] + e[0] * e[10] * e[34] + e[15] * e[34] * e[7] - 1.*e[15] * e[32] * e[5] + e[15] * e[35] * e[8] - 1.*e[15] * e[29] * e[2] - 1.*e[15] * e[28] * e[1] - 1.*e[15] * e[31] * e[4] + e[12] * e[30] * e[6] + e[12] * e[31] * e[7] + e[12] * e[4] * e[34] + e[12] * e[32] * e[8] + e[12] * e[5] * e[35] - 1.*e[33] * e[11] * e[2] - 1.*e[33] * e[10] * e[1] - 1.*e[33] * e[13] * e[4];
			A[6] = e[6] * e[1] * e[5] - e[6] * e[4] * e[2] + e[3] * e[7] * e[2] + e[0] * e[4] * e[8] - e[0] * e[5] * e[7] - e[3] * e[1] * e[8];
			A[148] = .5000000000*ep3[15] + e[9] * e[10] * e[16] - .5000000000*e[15] * ep2[11] + e[9] * e[11] * e[17] + .5000000000*ep2[12] * e[15] + .5000000000*e[15] * ep2[16] + .5000000000*e[15] * ep2[17] - .5000000000*e[15] * ep2[13] + .5000000000*ep2[9] * e[15] + e[12] * e[14] * e[17] - .5000000000*e[15] * ep2[10] - .5000000000*e[15] * ep2[14] + e[12] * e[13] * e[16];
			A[7] = e[15] * e[28] * e[14] - e[15] * e[13] * e[29] - e[15] * e[31] * e[11] + e[33] * e[10] * e[14] - e[33] * e[13] * e[11] + e[9] * e[13] * e[35] - e[9] * e[32] * e[16] + e[9] * e[31] * e[17] - e[9] * e[14] * e[34] + e[27] * e[13] * e[17] - e[27] * e[14] * e[16] + e[12] * e[34] * e[11] + e[12] * e[16] * e[29] - e[12] * e[10] * e[35] - e[12] * e[28] * e[17] + e[30] * e[16] * e[11] - e[30] * e[10] * e[17] + e[15] * e[10] * e[32];
			A[149] = e[18] * e[27] * e[24] + e[18] * e[28] * e[25] + e[18] * e[19] * e[34] + e[18] * e[29] * e[26] + e[18] * e[20] * e[35] + e[27] * e[19] * e[25] + e[27] * e[20] * e[26] + e[21] * e[30] * e[24] + e[21] * e[31] * e[25] + e[21] * e[22] * e[34] + e[21] * e[32] * e[26] + e[21] * e[23] * e[35] + e[30] * e[22] * e[25] + e[30] * e[23] * e[26] + e[24] * e[34] * e[25] + e[24] * e[35] * e[26] - 1.*e[24] * e[29] * e[20] - 1.*e[24] * e[31] * e[22] - 1.*e[24] * e[32] * e[23] - 1.*e[24] * e[28] * e[19] + 1.500000000*e[33] * ep2[24] + .5000000000*e[33] * ep2[25] + .5000000000*e[33] * ep2[26] - .5000000000*e[33] * ep2[23] - .5000000000*e[33] * ep2[19] - .5000000000*e[33] * ep2[20] - .5000000000*e[33] * ep2[22] + .5000000000*ep2[18] * e[33] + .5000000000*ep2[21] * e[33];
			A[9] = e[21] * e[25] * e[29] - e[27] * e[23] * e[25] + e[24] * e[19] * e[32] - e[21] * e[28] * e[26] - e[21] * e[19] * e[35] + e[18] * e[31] * e[26] - e[30] * e[19] * e[26] - e[24] * e[31] * e[20] + e[24] * e[28] * e[23] + e[27] * e[22] * e[26] + e[30] * e[25] * e[20] - e[33] * e[22] * e[20] + e[33] * e[19] * e[23] + e[21] * e[34] * e[20] - e[18] * e[23] * e[34] - e[24] * e[22] * e[29] - e[18] * e[32] * e[25] + e[18] * e[22] * e[35];
			A[155] = e[12] * e[14] * e[8] + e[12] * e[5] * e[17] + e[15] * e[16] * e[7] + e[15] * e[17] * e[8] + e[0] * e[11] * e[17] + e[0] * e[9] * e[15] + e[0] * e[10] * e[16] + e[3] * e[14] * e[17] + e[3] * e[13] * e[16] + e[9] * e[10] * e[7] + e[9] * e[1] * e[16] + e[9] * e[11] * e[8] + e[9] * e[2] * e[17] - 1.*e[15] * e[11] * e[2] - 1.*e[15] * e[10] * e[1] - 1.*e[15] * e[13] * e[4] - 1.*e[15] * e[14] * e[5] + e[12] * e[3] * e[15] + e[12] * e[13] * e[7] + e[12] * e[4] * e[16] + .5000000000*ep2[12] * e[6] + 1.500000000*ep2[15] * e[6] + .5000000000*e[6] * ep2[17] + .5000000000*e[6] * ep2[16] + .5000000000*e[6] * ep2[9] - .5000000000*e[6] * ep2[11] - .5000000000*e[6] * ep2[10] - .5000000000*e[6] * ep2[14] - .5000000000*e[6] * ep2[13];
			A[8] = -e[9] * e[14] * e[16] - e[12] * e[10] * e[17] + e[9] * e[13] * e[17] - e[15] * e[13] * e[11] + e[15] * e[10] * e[14] + e[12] * e[16] * e[11];
			A[154] = e[21] * e[14] * e[17] + e[21] * e[13] * e[16] + e[15] * e[26] * e[17] + e[15] * e[25] * e[16] - 1.*e[15] * e[23] * e[14] - 1.*e[15] * e[20] * e[11] - 1.*e[15] * e[19] * e[10] - 1.*e[15] * e[22] * e[13] + e[9] * e[20] * e[17] + e[9] * e[11] * e[26] + e[9] * e[19] * e[16] + e[9] * e[10] * e[25] + .5000000000*ep2[12] * e[24] + 1.500000000*e[24] * ep2[15] + .5000000000*e[24] * ep2[17] + .5000000000*e[24] * ep2[16] + .5000000000*ep2[9] * e[24] - .5000000000*e[24] * ep2[11] - .5000000000*e[24] * ep2[10] - .5000000000*e[24] * ep2[14] - .5000000000*e[24] * ep2[13] + e[18] * e[11] * e[17] + e[18] * e[9] * e[15] + e[18] * e[10] * e[16] + e[12] * e[21] * e[15] + e[12] * e[23] * e[17] + e[12] * e[14] * e[26] + e[12] * e[22] * e[16] + e[12] * e[13] * e[25];
			A[11] = -e[9] * e[5] * e[34] + e[9] * e[31] * e[8] - e[9] * e[32] * e[7] + e[27] * e[4] * e[17] + e[27] * e[13] * e[8] - e[27] * e[5] * e[16] - e[27] * e[14] * e[7] + e[0] * e[13] * e[35] - e[0] * e[32] * e[16] + e[0] * e[31] * e[17] - e[0] * e[14] * e[34] + e[9] * e[4] * e[35] + e[6] * e[10] * e[32] + e[6] * e[28] * e[14] - e[6] * e[13] * e[29] - e[6] * e[31] * e[11] + e[15] * e[1] * e[32] + e[3] * e[34] * e[11] + e[3] * e[16] * e[29] - e[3] * e[10] * e[35] - e[3] * e[28] * e[17] - e[12] * e[1] * e[35] + e[12] * e[7] * e[29] + e[12] * e[34] * e[2] - e[12] * e[28] * e[8] + e[15] * e[28] * e[5] - e[15] * e[4] * e[29] - e[15] * e[31] * e[2] + e[33] * e[1] * e[14] + e[33] * e[10] * e[5] - e[33] * e[4] * e[11] - e[33] * e[13] * e[2] + e[30] * e[7] * e[11] + e[30] * e[16] * e[2] - e[30] * e[1] * e[17] - e[30] * e[10] * e[8];
			A[153] = e[21] * e[31] * e[7] + e[21] * e[4] * e[34] + e[21] * e[32] * e[8] + e[21] * e[5] * e[35] + e[30] * e[22] * e[7] + e[30] * e[4] * e[25] + e[30] * e[23] * e[8] + e[30] * e[5] * e[26] + 3.*e[24] * e[33] * e[6] + e[24] * e[34] * e[7] + e[24] * e[35] * e[8] + e[33] * e[25] * e[7] + e[33] * e[26] * e[8] + e[0] * e[27] * e[24] + e[0] * e[18] * e[33] + e[0] * e[28] * e[25] + e[0] * e[19] * e[34] + e[0] * e[29] * e[26] + e[0] * e[20] * e[35] + e[18] * e[27] * e[6] + e[18] * e[28] * e[7] + e[18] * e[1] * e[34] + e[18] * e[29] * e[8] + e[18] * e[2] * e[35] + e[27] * e[19] * e[7] + e[27] * e[1] * e[25] + e[27] * e[20] * e[8] + e[27] * e[2] * e[26] + e[3] * e[30] * e[24] + e[3] * e[21] * e[33] + e[3] * e[31] * e[25] + e[3] * e[22] * e[34] + e[3] * e[32] * e[26] + e[3] * e[23] * e[35] + e[6] * e[30] * e[21] - 1.*e[6] * e[29] * e[20] + e[6] * e[35] * e[26] - 1.*e[6] * e[31] * e[22] - 1.*e[6] * e[32] * e[23] - 1.*e[6] * e[28] * e[19] + e[6] * e[34] * e[25] - 1.*e[24] * e[32] * e[5] - 1.*e[24] * e[29] * e[2] - 1.*e[24] * e[28] * e[1] - 1.*e[24] * e[31] * e[4] - 1.*e[33] * e[20] * e[2] - 1.*e[33] * e[19] * e[1] - 1.*e[33] * e[22] * e[4] - 1.*e[33] * e[23] * e[5];
			A[10] = e[21] * e[25] * e[20] - e[21] * e[19] * e[26] + e[18] * e[22] * e[26] - e[18] * e[23] * e[25] - e[24] * e[22] * e[20] + e[24] * e[19] * e[23];
			A[152] = e[3] * e[4] * e[25] + e[3] * e[23] * e[8] + e[3] * e[5] * e[26] + e[21] * e[4] * e[7] + e[21] * e[5] * e[8] + e[6] * e[25] * e[7] + e[6] * e[26] * e[8] + e[0] * e[19] * e[7] + e[0] * e[1] * e[25] + e[0] * e[20] * e[8] + e[0] * e[2] * e[26] - 1.*e[6] * e[20] * e[2] - 1.*e[6] * e[19] * e[1] - 1.*e[6] * e[22] * e[4] - 1.*e[6] * e[23] * e[5] + e[18] * e[1] * e[7] + e[18] * e[0] * e[6] + e[18] * e[2] * e[8] + e[3] * e[21] * e[6] + e[3] * e[22] * e[7] - .5000000000*e[24] * ep2[4] + .5000000000*e[24] * ep2[0] + 1.500000000*e[24] * ep2[6] - .5000000000*e[24] * ep2[5] - .5000000000*e[24] * ep2[1] + .5000000000*e[24] * ep2[7] + .5000000000*e[24] * ep2[3] - .5000000000*e[24] * ep2[2] + .5000000000*e[24] * ep2[8];
			A[13] = e[6] * e[28] * e[23] - e[6] * e[22] * e[29] - e[6] * e[31] * e[20] - e[3] * e[19] * e[35] + e[3] * e[34] * e[20] + e[3] * e[25] * e[29] - e[21] * e[1] * e[35] + e[21] * e[7] * e[29] + e[21] * e[34] * e[2] + e[24] * e[1] * e[32] + e[24] * e[28] * e[5] - e[24] * e[4] * e[29] - e[24] * e[31] * e[2] + e[33] * e[1] * e[23] + e[33] * e[19] * e[5] - e[33] * e[4] * e[20] - e[33] * e[22] * e[2] - e[21] * e[28] * e[8] + e[30] * e[7] * e[20] + e[30] * e[25] * e[2] - e[30] * e[1] * e[26] + e[18] * e[4] * e[35] - e[18] * e[5] * e[34] + e[18] * e[31] * e[8] - e[18] * e[32] * e[7] + e[27] * e[4] * e[26] + e[27] * e[22] * e[8] - e[27] * e[5] * e[25] - e[27] * e[23] * e[7] - e[3] * e[28] * e[26] - e[0] * e[32] * e[25] + e[0] * e[22] * e[35] - e[0] * e[23] * e[34] + e[0] * e[31] * e[26] - e[30] * e[19] * e[8] + e[6] * e[19] * e[32];
			A[159] = .5000000000*ep2[18] * e[6] + .5000000000*ep2[21] * e[6] + 1.500000000*ep2[24] * e[6] + .5000000000*e[6] * ep2[26] - .5000000000*e[6] * ep2[23] - .5000000000*e[6] * ep2[19] - .5000000000*e[6] * ep2[20] - .5000000000*e[6] * ep2[22] + .5000000000*e[6] * ep2[25] + e[21] * e[3] * e[24] + e[18] * e[20] * e[8] + e[21] * e[4] * e[25] + e[18] * e[19] * e[7] + e[18] * e[1] * e[25] + e[21] * e[22] * e[7] + e[21] * e[23] * e[8] + e[18] * e[0] * e[24] + e[18] * e[2] * e[26] + e[21] * e[5] * e[26] + e[24] * e[26] * e[8] - 1.*e[24] * e[20] * e[2] - 1.*e[24] * e[19] * e[1] - 1.*e[24] * e[22] * e[4] + e[24] * e[25] * e[7] - 1.*e[24] * e[23] * e[5] + e[0] * e[19] * e[25] + e[0] * e[20] * e[26] + e[3] * e[22] * e[25] + e[3] * e[23] * e[26];
			A[12] = e[18] * e[4] * e[8] + e[3] * e[7] * e[20] + e[3] * e[25] * e[2] - e[3] * e[1] * e[26] - e[18] * e[5] * e[7] + e[6] * e[1] * e[23] + e[6] * e[19] * e[5] - e[6] * e[4] * e[20] - e[6] * e[22] * e[2] + e[21] * e[7] * e[2] - e[21] * e[1] * e[8] + e[24] * e[1] * e[5] - e[24] * e[4] * e[2] - e[3] * e[19] * e[8] + e[0] * e[4] * e[26] + e[0] * e[22] * e[8] - e[0] * e[5] * e[25] - e[0] * e[23] * e[7];
			A[158] = e[9] * e[1] * e[7] + e[9] * e[0] * e[6] + e[9] * e[2] * e[8] + e[3] * e[12] * e[6] + e[3] * e[13] * e[7] + e[3] * e[4] * e[16] + e[3] * e[14] * e[8] + e[3] * e[5] * e[17] + e[12] * e[4] * e[7] + e[12] * e[5] * e[8] + e[6] * e[16] * e[7] + e[6] * e[17] * e[8] - 1.*e[6] * e[11] * e[2] - 1.*e[6] * e[10] * e[1] - 1.*e[6] * e[13] * e[4] - 1.*e[6] * e[14] * e[5] + e[0] * e[10] * e[7] + e[0] * e[1] * e[16] + e[0] * e[11] * e[8] + e[0] * e[2] * e[17] + .5000000000*ep2[3] * e[15] + 1.500000000*e[15] * ep2[6] + .5000000000*e[15] * ep2[7] + .5000000000*e[15] * ep2[8] + .5000000000*ep2[0] * e[15] - .5000000000*e[15] * ep2[4] - .5000000000*e[15] * ep2[5] - .5000000000*e[15] * ep2[1] - .5000000000*e[15] * ep2[2];
			A[15] = -e[15] * e[13] * e[2] - e[6] * e[13] * e[11] - e[15] * e[4] * e[11] + e[12] * e[16] * e[2] - e[3] * e[10] * e[17] + e[3] * e[16] * e[11] + e[0] * e[13] * e[17] - e[0] * e[14] * e[16] + e[15] * e[1] * e[14] - e[12] * e[10] * e[8] + e[9] * e[4] * e[17] + e[9] * e[13] * e[8] - e[9] * e[5] * e[16] - e[9] * e[14] * e[7] + e[15] * e[10] * e[5] + e[12] * e[7] * e[11] + e[6] * e[10] * e[14] - e[12] * e[1] * e[17];
			A[157] = e[12] * e[30] * e[24] + e[12] * e[21] * e[33] + e[12] * e[31] * e[25] + e[12] * e[22] * e[34] + e[12] * e[32] * e[26] + e[12] * e[23] * e[35] + e[9] * e[27] * e[24] + e[9] * e[18] * e[33] + e[9] * e[28] * e[25] + e[9] * e[19] * e[34] + e[9] * e[29] * e[26] + e[9] * e[20] * e[35] + e[21] * e[30] * e[15] + e[21] * e[32] * e[17] + e[21] * e[14] * e[35] + e[21] * e[31] * e[16] + e[21] * e[13] * e[34] + e[30] * e[23] * e[17] + e[30] * e[14] * e[26] + e[30] * e[22] * e[16] + e[30] * e[13] * e[25] + e[15] * e[27] * e[18] + 3.*e[15] * e[33] * e[24] - 1.*e[15] * e[29] * e[20] + e[15] * e[35] * e[26] - 1.*e[15] * e[31] * e[22] - 1.*e[15] * e[32] * e[23] - 1.*e[15] * e[28] * e[19] + e[15] * e[34] * e[25] + e[18] * e[29] * e[17] + e[18] * e[11] * e[35] + e[18] * e[28] * e[16] + e[18] * e[10] * e[34] + e[27] * e[20] * e[17] + e[27] * e[11] * e[26] + e[27] * e[19] * e[16] + e[27] * e[10] * e[25] - 1.*e[24] * e[28] * e[10] - 1.*e[24] * e[31] * e[13] - 1.*e[24] * e[32] * e[14] + e[24] * e[34] * e[16] + e[24] * e[35] * e[17] - 1.*e[24] * e[29] * e[11] - 1.*e[33] * e[23] * e[14] + e[33] * e[25] * e[16] + e[33] * e[26] * e[17] - 1.*e[33] * e[20] * e[11] - 1.*e[33] * e[19] * e[10] - 1.*e[33] * e[22] * e[13];
			A[14] = e[18] * e[13] * e[17] + e[9] * e[13] * e[26] + e[9] * e[22] * e[17] - e[9] * e[14] * e[25] - e[18] * e[14] * e[16] - e[15] * e[13] * e[20] - e[15] * e[22] * e[11] + e[12] * e[16] * e[20] + e[12] * e[25] * e[11] - e[12] * e[10] * e[26] - e[12] * e[19] * e[17] + e[21] * e[16] * e[11] - e[21] * e[10] * e[17] - e[9] * e[23] * e[16] + e[24] * e[10] * e[14] - e[24] * e[13] * e[11] + e[15] * e[10] * e[23] + e[15] * e[19] * e[14];
			A[156] = e[21] * e[12] * e[24] + e[21] * e[23] * e[17] + e[21] * e[14] * e[26] + e[21] * e[22] * e[16] + e[21] * e[13] * e[25] + e[24] * e[26] * e[17] + e[24] * e[25] * e[16] + e[9] * e[19] * e[25] + e[9] * e[18] * e[24] + e[9] * e[20] * e[26] + e[12] * e[22] * e[25] + e[12] * e[23] * e[26] + e[18] * e[20] * e[17] + e[18] * e[11] * e[26] + e[18] * e[19] * e[16] + e[18] * e[10] * e[25] - 1.*e[24] * e[23] * e[14] - 1.*e[24] * e[20] * e[11] - 1.*e[24] * e[19] * e[10] - 1.*e[24] * e[22] * e[13] + .5000000000*ep2[21] * e[15] + 1.500000000*ep2[24] * e[15] + .5000000000*e[15] * ep2[25] + .5000000000*e[15] * ep2[26] + .5000000000*e[15] * ep2[18] - .5000000000*e[15] * ep2[23] - .5000000000*e[15] * ep2[19] - .5000000000*e[15] * ep2[20] - .5000000000*e[15] * ep2[22];
			A[18] = e[6] * e[1] * e[14] + e[15] * e[1] * e[5] - e[0] * e[5] * e[16] - e[0] * e[14] * e[7] + e[0] * e[13] * e[8] - e[15] * e[4] * e[2] + e[12] * e[7] * e[2] + e[6] * e[10] * e[5] + e[3] * e[7] * e[11] - e[6] * e[4] * e[11] + e[3] * e[16] * e[2] - e[6] * e[13] * e[2] - e[3] * e[1] * e[17] - e[9] * e[5] * e[7] - e[3] * e[10] * e[8] - e[12] * e[1] * e[8] + e[0] * e[4] * e[17] + e[9] * e[4] * e[8];
			A[128] = -.5000000000*e[14] * ep2[16] - .5000000000*e[14] * ep2[10] - .5000000000*e[14] * ep2[9] + e[11] * e[9] * e[12] + .5000000000*ep3[14] + e[17] * e[13] * e[16] + .5000000000*e[14] * ep2[12] + e[11] * e[10] * e[13] - .5000000000*e[14] * ep2[15] + .5000000000*e[14] * ep2[17] + e[17] * e[12] * e[15] + .5000000000*ep2[11] * e[14] + .5000000000*e[14] * ep2[13];
			A[19] = -e[21] * e[19] * e[8] + e[18] * e[4] * e[26] - e[18] * e[5] * e[25] - e[18] * e[23] * e[7] + e[21] * e[25] * e[2] - e[21] * e[1] * e[26] + e[6] * e[19] * e[23] + e[18] * e[22] * e[8] - e[0] * e[23] * e[25] - e[6] * e[22] * e[20] + e[24] * e[1] * e[23] + e[24] * e[19] * e[5] - e[24] * e[4] * e[20] - e[24] * e[22] * e[2] + e[3] * e[25] * e[20] - e[3] * e[19] * e[26] + e[0] * e[22] * e[26] + e[21] * e[7] * e[20];
			A[129] = .5000000000*ep2[20] * e[32] + 1.500000000*e[32] * ep2[23] + .5000000000*e[32] * ep2[22] + .5000000000*e[32] * ep2[21] + .5000000000*e[32] * ep2[26] - .5000000000*e[32] * ep2[18] - .5000000000*e[32] * ep2[19] - .5000000000*e[32] * ep2[24] - .5000000000*e[32] * ep2[25] + e[20] * e[27] * e[21] + e[20] * e[18] * e[30] + e[20] * e[28] * e[22] + e[20] * e[19] * e[31] + e[20] * e[29] * e[23] + e[29] * e[19] * e[22] + e[29] * e[18] * e[21] + e[23] * e[30] * e[21] + e[23] * e[31] * e[22] + e[26] * e[30] * e[24] + e[26] * e[21] * e[33] + e[26] * e[31] * e[25] + e[26] * e[22] * e[34] + e[26] * e[23] * e[35] + e[35] * e[22] * e[25] + e[35] * e[21] * e[24] - 1.*e[23] * e[27] * e[18] - 1.*e[23] * e[33] * e[24] - 1.*e[23] * e[28] * e[19] - 1.*e[23] * e[34] * e[25];
			A[16] = -e[9] * e[23] * e[25] - e[21] * e[10] * e[26] - e[21] * e[19] * e[17] - e[18] * e[23] * e[16] + e[18] * e[13] * e[26] + e[12] * e[25] * e[20] - e[12] * e[19] * e[26] - e[15] * e[22] * e[20] + e[21] * e[16] * e[20] + e[21] * e[25] * e[11] + e[24] * e[10] * e[23] + e[24] * e[19] * e[14] - e[24] * e[13] * e[20] - e[24] * e[22] * e[11] + e[18] * e[22] * e[17] - e[18] * e[14] * e[25] + e[9] * e[22] * e[26] + e[15] * e[19] * e[23];
			A[130] = .5000000000*e[23] * ep2[21] + e[20] * e[19] * e[22] + e[20] * e[18] * e[21] + .5000000000*ep3[23] + e[26] * e[22] * e[25] + .5000000000*e[23] * ep2[26] - .5000000000*e[23] * ep2[18] + .5000000000*e[23] * ep2[22] - .5000000000*e[23] * ep2[19] + e[26] * e[21] * e[24] + .5000000000*ep2[20] * e[23] - .5000000000*e[23] * ep2[24] - .5000000000*e[23] * ep2[25];
			A[17] = e[18] * e[13] * e[35] - e[18] * e[32] * e[16] + e[18] * e[31] * e[17] - e[18] * e[14] * e[34] + e[27] * e[13] * e[26] + e[27] * e[22] * e[17] - e[27] * e[14] * e[25] - e[27] * e[23] * e[16] - e[9] * e[32] * e[25] + e[9] * e[22] * e[35] - e[9] * e[23] * e[34] + e[9] * e[31] * e[26] + e[15] * e[19] * e[32] + e[15] * e[28] * e[23] - e[15] * e[22] * e[29] - e[15] * e[31] * e[20] + e[24] * e[10] * e[32] + e[24] * e[28] * e[14] - e[24] * e[13] * e[29] - e[24] * e[31] * e[11] + e[33] * e[10] * e[23] + e[33] * e[19] * e[14] - e[33] * e[13] * e[20] - e[33] * e[22] * e[11] + e[21] * e[16] * e[29] - e[21] * e[10] * e[35] - e[21] * e[28] * e[17] + e[30] * e[16] * e[20] + e[30] * e[25] * e[11] - e[30] * e[10] * e[26] - e[30] * e[19] * e[17] - e[12] * e[28] * e[26] - e[12] * e[19] * e[35] + e[12] * e[34] * e[20] + e[12] * e[25] * e[29] + e[21] * e[34] * e[11];
			A[131] = -1.*e[32] * e[10] * e[1] + e[32] * e[13] * e[4] - 1.*e[32] * e[16] * e[7] - 1.*e[32] * e[15] * e[6] - 1.*e[32] * e[9] * e[0] + e[32] * e[12] * e[3] + e[17] * e[30] * e[6] + e[17] * e[3] * e[33] + e[17] * e[31] * e[7] + e[17] * e[4] * e[34] + e[17] * e[5] * e[35] - 1.*e[5] * e[27] * e[9] - 1.*e[5] * e[28] * e[10] - 1.*e[5] * e[33] * e[15] - 1.*e[5] * e[34] * e[16] + e[5] * e[29] * e[11] + e[35] * e[12] * e[6] + e[35] * e[3] * e[15] + e[35] * e[13] * e[7] + e[35] * e[4] * e[16] + e[11] * e[27] * e[3] + e[11] * e[0] * e[30] + e[11] * e[28] * e[4] + e[11] * e[1] * e[31] + e[29] * e[9] * e[3] + e[29] * e[0] * e[12] + e[29] * e[10] * e[4] + e[29] * e[1] * e[13] + e[5] * e[30] * e[12] + 3.*e[5] * e[32] * e[14] + e[5] * e[31] * e[13] + e[8] * e[30] * e[15] + e[8] * e[12] * e[33] + e[8] * e[32] * e[17] + e[8] * e[14] * e[35] + e[8] * e[31] * e[16] + e[8] * e[13] * e[34] + e[2] * e[27] * e[12] + e[2] * e[9] * e[30] + e[2] * e[29] * e[14] + e[2] * e[11] * e[32] + e[2] * e[28] * e[13] + e[2] * e[10] * e[31] - 1.*e[14] * e[27] * e[0] - 1.*e[14] * e[34] * e[7] - 1.*e[14] * e[33] * e[6] + e[14] * e[30] * e[3] - 1.*e[14] * e[28] * e[1] + e[14] * e[31] * e[4];
			A[22] = .5000000000*e[18] * ep2[29] + .5000000000*e[18] * ep2[28] + .5000000000*e[18] * ep2[30] + .5000000000*e[18] * ep2[33] - .5000000000*e[18] * ep2[32] - .5000000000*e[18] * ep2[31] - .5000000000*e[18] * ep2[34] - .5000000000*e[18] * ep2[35] + 1.500000000*e[18] * ep2[27] + e[27] * e[28] * e[19] + e[27] * e[29] * e[20] + e[21] * e[27] * e[30] + e[21] * e[29] * e[32] + e[21] * e[28] * e[31] + e[30] * e[28] * e[22] + e[30] * e[19] * e[31] + e[30] * e[29] * e[23] + e[30] * e[20] * e[32] + e[24] * e[27] * e[33] + e[24] * e[29] * e[35] + e[24] * e[28] * e[34] + e[33] * e[28] * e[25] + e[33] * e[19] * e[34] + e[33] * e[29] * e[26] + e[33] * e[20] * e[35] - 1.*e[27] * e[35] * e[26] - 1.*e[27] * e[31] * e[22] - 1.*e[27] * e[32] * e[23] - 1.*e[27] * e[34] * e[25];
			A[132] = e[20] * e[1] * e[4] + e[20] * e[0] * e[3] + e[20] * e[2] * e[5] + e[5] * e[21] * e[3] + e[5] * e[22] * e[4] + e[8] * e[21] * e[6] + e[8] * e[3] * e[24] + e[8] * e[22] * e[7] + e[8] * e[4] * e[25] + e[8] * e[5] * e[26] + e[26] * e[4] * e[7] + e[26] * e[3] * e[6] + e[2] * e[18] * e[3] + e[2] * e[0] * e[21] + e[2] * e[19] * e[4] + e[2] * e[1] * e[22] - 1.*e[5] * e[19] * e[1] - 1.*e[5] * e[18] * e[0] - 1.*e[5] * e[25] * e[7] - 1.*e[5] * e[24] * e[6] + .5000000000*e[23] * ep2[4] - .5000000000*e[23] * ep2[0] - .5000000000*e[23] * ep2[6] + 1.500000000*e[23] * ep2[5] - .5000000000*e[23] * ep2[1] - .5000000000*e[23] * ep2[7] + .5000000000*e[23] * ep2[3] + .5000000000*e[23] * ep2[2] + .5000000000*e[23] * ep2[8];
			A[23] = 1.500000000*e[9] * ep2[27] + .5000000000*e[9] * ep2[29] + .5000000000*e[9] * ep2[28] - .5000000000*e[9] * ep2[32] - .5000000000*e[9] * ep2[31] + .5000000000*e[9] * ep2[33] + .5000000000*e[9] * ep2[30] - .5000000000*e[9] * ep2[34] - .5000000000*e[9] * ep2[35] + e[33] * e[27] * e[15] + e[33] * e[29] * e[17] + e[33] * e[11] * e[35] + e[33] * e[28] * e[16] + e[33] * e[10] * e[34] + e[27] * e[29] * e[11] + e[27] * e[28] * e[10] + e[27] * e[30] * e[12] - 1.*e[27] * e[31] * e[13] - 1.*e[27] * e[32] * e[14] - 1.*e[27] * e[34] * e[16] - 1.*e[27] * e[35] * e[17] + e[30] * e[29] * e[14] + e[30] * e[11] * e[32] + e[30] * e[28] * e[13] + e[30] * e[10] * e[31] + e[12] * e[29] * e[32] + e[12] * e[28] * e[31] + e[15] * e[29] * e[35] + e[15] * e[28] * e[34];
			A[133] = -1.*e[32] * e[24] * e[6] + e[8] * e[30] * e[24] + e[8] * e[21] * e[33] + e[8] * e[31] * e[25] + e[8] * e[22] * e[34] + e[26] * e[30] * e[6] + e[26] * e[3] * e[33] + e[26] * e[31] * e[7] + e[26] * e[4] * e[34] + e[26] * e[32] * e[8] + e[26] * e[5] * e[35] + e[35] * e[21] * e[6] + e[35] * e[3] * e[24] + e[35] * e[22] * e[7] + e[35] * e[4] * e[25] + e[35] * e[23] * e[8] + e[2] * e[27] * e[21] + e[2] * e[18] * e[30] + e[2] * e[28] * e[22] + e[2] * e[19] * e[31] + e[2] * e[29] * e[23] + e[2] * e[20] * e[32] + e[20] * e[27] * e[3] + e[20] * e[0] * e[30] + e[20] * e[28] * e[4] + e[20] * e[1] * e[31] + e[20] * e[29] * e[5] + e[29] * e[18] * e[3] + e[29] * e[0] * e[21] + e[29] * e[19] * e[4] + e[29] * e[1] * e[22] + e[5] * e[30] * e[21] + e[5] * e[31] * e[22] + 3.*e[5] * e[32] * e[23] - 1.*e[5] * e[27] * e[18] - 1.*e[5] * e[33] * e[24] - 1.*e[5] * e[28] * e[19] - 1.*e[5] * e[34] * e[25] - 1.*e[23] * e[27] * e[0] - 1.*e[23] * e[34] * e[7] - 1.*e[23] * e[33] * e[6] + e[23] * e[30] * e[3] - 1.*e[23] * e[28] * e[1] + e[23] * e[31] * e[4] + e[32] * e[21] * e[3] - 1.*e[32] * e[19] * e[1] + e[32] * e[22] * e[4] - 1.*e[32] * e[18] * e[0] - 1.*e[32] * e[25] * e[7];
			A[20] = .5000000000*e[27] * ep2[33] - .5000000000*e[27] * ep2[32] - .5000000000*e[27] * ep2[31] - .5000000000*e[27] * ep2[34] - .5000000000*e[27] * ep2[35] + e[33] * e[29] * e[35] + .5000000000*e[27] * ep2[29] + e[30] * e[29] * e[32] + e[30] * e[28] * e[31] + e[33] * e[28] * e[34] + .5000000000*e[27] * ep2[28] + .5000000000*e[27] * ep2[30] + .5000000000*ep3[27];
			A[134] = e[14] * e[21] * e[12] + e[14] * e[22] * e[13] + e[17] * e[21] * e[15] + e[17] * e[12] * e[24] + e[17] * e[14] * e[26] + e[17] * e[22] * e[16] + e[17] * e[13] * e[25] + e[26] * e[12] * e[15] + e[26] * e[13] * e[16] - 1.*e[14] * e[24] * e[15] - 1.*e[14] * e[25] * e[16] - 1.*e[14] * e[18] * e[9] - 1.*e[14] * e[19] * e[10] + e[11] * e[18] * e[12] + e[11] * e[9] * e[21] + e[11] * e[19] * e[13] + e[11] * e[10] * e[22] + e[20] * e[11] * e[14] + e[20] * e[9] * e[12] + e[20] * e[10] * e[13] + 1.500000000*e[23] * ep2[14] + .5000000000*e[23] * ep2[12] + .5000000000*e[23] * ep2[13] + .5000000000*e[23] * ep2[17] + .5000000000*ep2[11] * e[23] - .5000000000*e[23] * ep2[16] - .5000000000*e[23] * ep2[9] - .5000000000*e[23] * ep2[15] - .5000000000*e[23] * ep2[10];
			A[21] = 1.500000000*e[0] * ep2[27] + .5000000000*e[0] * ep2[29] + .5000000000*e[0] * ep2[28] + .5000000000*e[0] * ep2[30] - .5000000000*e[0] * ep2[32] - .5000000000*e[0] * ep2[31] + .5000000000*e[0] * ep2[33] - .5000000000*e[0] * ep2[34] - .5000000000*e[0] * ep2[35] - 1.*e[27] * e[31] * e[4] + e[3] * e[27] * e[30] + e[3] * e[29] * e[32] + e[3] * e[28] * e[31] + e[30] * e[28] * e[4] + e[30] * e[1] * e[31] + e[30] * e[29] * e[5] + e[30] * e[2] * e[32] + e[6] * e[27] * e[33] + e[6] * e[29] * e[35] + e[6] * e[28] * e[34] + e[27] * e[28] * e[1] + e[27] * e[29] * e[2] + e[33] * e[28] * e[7] + e[33] * e[1] * e[34] + e[33] * e[29] * e[8] + e[33] * e[2] * e[35] - 1.*e[27] * e[34] * e[7] - 1.*e[27] * e[32] * e[5] - 1.*e[27] * e[35] * e[8];
			A[135] = e[14] * e[12] * e[3] + e[14] * e[13] * e[4] + e[17] * e[12] * e[6] + e[17] * e[3] * e[15] + e[17] * e[13] * e[7] + e[17] * e[4] * e[16] + e[17] * e[14] * e[8] + e[8] * e[12] * e[15] + e[8] * e[13] * e[16] + e[2] * e[11] * e[14] + e[2] * e[9] * e[12] + e[2] * e[10] * e[13] + e[11] * e[9] * e[3] + e[11] * e[0] * e[12] + e[11] * e[10] * e[4] + e[11] * e[1] * e[13] - 1.*e[14] * e[10] * e[1] - 1.*e[14] * e[16] * e[7] - 1.*e[14] * e[15] * e[6] - 1.*e[14] * e[9] * e[0] - .5000000000*e[5] * ep2[16] - .5000000000*e[5] * ep2[9] + .5000000000*e[5] * ep2[11] + .5000000000*e[5] * ep2[12] - .5000000000*e[5] * ep2[15] - .5000000000*e[5] * ep2[10] + .5000000000*e[5] * ep2[13] + 1.500000000*ep2[14] * e[5] + .5000000000*e[5] * ep2[17];
			A[27] = 1.500000000*e[27] * ep2[9] - .5000000000*e[27] * ep2[16] + .5000000000*e[27] * ep2[11] + .5000000000*e[27] * ep2[12] + .5000000000*e[27] * ep2[15] - .5000000000*e[27] * ep2[17] + .5000000000*e[27] * ep2[10] - .5000000000*e[27] * ep2[14] - .5000000000*e[27] * ep2[13] + e[12] * e[10] * e[31] + e[30] * e[11] * e[14] + e[30] * e[10] * e[13] + e[15] * e[9] * e[33] + e[15] * e[29] * e[17] + e[15] * e[11] * e[35] + e[15] * e[28] * e[16] + e[15] * e[10] * e[34] + e[33] * e[11] * e[17] + e[33] * e[10] * e[16] - 1.*e[9] * e[31] * e[13] - 1.*e[9] * e[32] * e[14] - 1.*e[9] * e[34] * e[16] - 1.*e[9] * e[35] * e[17] + e[9] * e[29] * e[11] + e[9] * e[28] * e[10] + e[12] * e[9] * e[30] + e[12] * e[29] * e[14] + e[12] * e[11] * e[32] + e[12] * e[28] * e[13];
			A[137] = e[29] * e[18] * e[12] + e[29] * e[9] * e[21] + e[29] * e[19] * e[13] + e[29] * e[10] * e[22] + e[17] * e[30] * e[24] + e[17] * e[21] * e[33] + e[17] * e[31] * e[25] + e[17] * e[22] * e[34] + e[17] * e[32] * e[26] + e[17] * e[23] * e[35] - 1.*e[23] * e[27] * e[9] - 1.*e[23] * e[28] * e[10] - 1.*e[23] * e[33] * e[15] - 1.*e[23] * e[34] * e[16] - 1.*e[32] * e[24] * e[15] - 1.*e[32] * e[25] * e[16] - 1.*e[32] * e[18] * e[9] - 1.*e[32] * e[19] * e[10] + e[26] * e[30] * e[15] + e[26] * e[12] * e[33] + e[26] * e[31] * e[16] + e[26] * e[13] * e[34] + e[35] * e[21] * e[15] + e[35] * e[12] * e[24] + e[35] * e[22] * e[16] + e[35] * e[13] * e[25] + e[14] * e[30] * e[21] + e[14] * e[31] * e[22] + 3.*e[14] * e[32] * e[23] + e[11] * e[27] * e[21] + e[11] * e[18] * e[30] + e[11] * e[28] * e[22] + e[11] * e[19] * e[31] + e[11] * e[29] * e[23] + e[11] * e[20] * e[32] + e[23] * e[30] * e[12] + e[23] * e[31] * e[13] + e[32] * e[21] * e[12] + e[32] * e[22] * e[13] - 1.*e[14] * e[27] * e[18] - 1.*e[14] * e[33] * e[24] + e[14] * e[29] * e[20] + e[14] * e[35] * e[26] - 1.*e[14] * e[28] * e[19] - 1.*e[14] * e[34] * e[25] + e[20] * e[27] * e[12] + e[20] * e[9] * e[30] + e[20] * e[28] * e[13] + e[20] * e[10] * e[31];
			A[26] = .5000000000*e[0] * ep2[1] + .5000000000*e[0] * ep2[2] + e[6] * e[2] * e[8] + e[6] * e[1] * e[7] + .5000000000*e[0] * ep2[3] + e[3] * e[1] * e[4] + .5000000000*e[0] * ep2[6] + e[3] * e[2] * e[5] - .5000000000*e[0] * ep2[5] - .5000000000*e[0] * ep2[8] + .5000000000*ep3[0] - .5000000000*e[0] * ep2[7] - .5000000000*e[0] * ep2[4];
			A[136] = 1.500000000*ep2[23] * e[14] + .5000000000*e[14] * ep2[26] - .5000000000*e[14] * ep2[18] - .5000000000*e[14] * ep2[19] + .5000000000*e[14] * ep2[20] + .5000000000*e[14] * ep2[22] - .5000000000*e[14] * ep2[24] + .5000000000*e[14] * ep2[21] - .5000000000*e[14] * ep2[25] + e[23] * e[21] * e[12] + e[23] * e[22] * e[13] + e[26] * e[21] * e[15] + e[26] * e[12] * e[24] + e[26] * e[23] * e[17] + e[26] * e[22] * e[16] + e[26] * e[13] * e[25] + e[17] * e[22] * e[25] + e[17] * e[21] * e[24] + e[11] * e[19] * e[22] + e[11] * e[18] * e[21] + e[11] * e[20] * e[23] + e[20] * e[18] * e[12] + e[20] * e[9] * e[21] + e[20] * e[19] * e[13] + e[20] * e[10] * e[22] - 1.*e[23] * e[24] * e[15] - 1.*e[23] * e[25] * e[16] - 1.*e[23] * e[18] * e[9] - 1.*e[23] * e[19] * e[10];
			A[25] = 1.500000000*e[27] * ep2[0] - .5000000000*e[27] * ep2[4] + .5000000000*e[27] * ep2[6] - .5000000000*e[27] * ep2[5] + .5000000000*e[27] * ep2[1] - .5000000000*e[27] * ep2[7] + .5000000000*e[27] * ep2[3] + .5000000000*e[27] * ep2[2] - .5000000000*e[27] * ep2[8] + e[0] * e[33] * e[6] + e[0] * e[30] * e[3] - 1.*e[0] * e[35] * e[8] - 1.*e[0] * e[31] * e[4] + e[3] * e[28] * e[4] + e[3] * e[1] * e[31] + e[3] * e[29] * e[5] + e[3] * e[2] * e[32] + e[30] * e[1] * e[4] + e[30] * e[2] * e[5] + e[6] * e[28] * e[7] + e[6] * e[1] * e[34] + e[6] * e[29] * e[8] + e[6] * e[2] * e[35] + e[33] * e[1] * e[7] + e[33] * e[2] * e[8] + e[0] * e[28] * e[1] + e[0] * e[29] * e[2] - 1.*e[0] * e[34] * e[7] - 1.*e[0] * e[32] * e[5];
			A[139] = e[8] * e[22] * e[25] + e[8] * e[21] * e[24] + e[20] * e[18] * e[3] + e[20] * e[0] * e[21] + e[20] * e[19] * e[4] + e[20] * e[1] * e[22] + e[20] * e[2] * e[23] + e[23] * e[21] * e[3] + e[23] * e[22] * e[4] + e[23] * e[26] * e[8] - 1.*e[23] * e[19] * e[1] - 1.*e[23] * e[18] * e[0] - 1.*e[23] * e[25] * e[7] - 1.*e[23] * e[24] * e[6] + e[2] * e[19] * e[22] + e[2] * e[18] * e[21] + e[26] * e[21] * e[6] + e[26] * e[3] * e[24] + e[26] * e[22] * e[7] + e[26] * e[4] * e[25] + .5000000000*ep2[20] * e[5] + 1.500000000*ep2[23] * e[5] + .5000000000*e[5] * ep2[22] + .5000000000*e[5] * ep2[21] + .5000000000*e[5] * ep2[26] - .5000000000*e[5] * ep2[18] - .5000000000*e[5] * ep2[19] - .5000000000*e[5] * ep2[24] - .5000000000*e[5] * ep2[25];
			A[24] = e[24] * e[11] * e[8] + e[24] * e[2] * e[17] + 3.*e[9] * e[18] * e[0] + e[9] * e[19] * e[1] + e[9] * e[20] * e[2] + e[18] * e[10] * e[1] + e[18] * e[11] * e[2] + e[3] * e[18] * e[12] + e[3] * e[9] * e[21] + e[3] * e[20] * e[14] + e[3] * e[11] * e[23] + e[3] * e[19] * e[13] + e[3] * e[10] * e[22] + e[6] * e[18] * e[15] + e[6] * e[9] * e[24] + e[6] * e[20] * e[17] + e[6] * e[11] * e[26] + e[6] * e[19] * e[16] + e[6] * e[10] * e[25] + e[0] * e[20] * e[11] + e[0] * e[19] * e[10] - 1.*e[9] * e[26] * e[8] - 1.*e[9] * e[22] * e[4] - 1.*e[9] * e[25] * e[7] - 1.*e[9] * e[23] * e[5] + e[12] * e[0] * e[21] + e[12] * e[19] * e[4] + e[12] * e[1] * e[22] + e[12] * e[20] * e[5] + e[12] * e[2] * e[23] - 1.*e[18] * e[13] * e[4] - 1.*e[18] * e[16] * e[7] - 1.*e[18] * e[14] * e[5] - 1.*e[18] * e[17] * e[8] + e[21] * e[10] * e[4] + e[21] * e[1] * e[13] + e[21] * e[11] * e[5] + e[21] * e[2] * e[14] + e[15] * e[0] * e[24] + e[15] * e[19] * e[7] + e[15] * e[1] * e[25] + e[15] * e[20] * e[8] + e[15] * e[2] * e[26] - 1.*e[0] * e[23] * e[14] - 1.*e[0] * e[25] * e[16] - 1.*e[0] * e[26] * e[17] - 1.*e[0] * e[22] * e[13] + e[24] * e[10] * e[7] + e[24] * e[1] * e[16];
			A[138] = e[11] * e[1] * e[4] + e[11] * e[0] * e[3] + e[11] * e[2] * e[5] + e[5] * e[12] * e[3] + e[5] * e[13] * e[4] + e[8] * e[12] * e[6] + e[8] * e[3] * e[15] + e[8] * e[13] * e[7] + e[8] * e[4] * e[16] + e[8] * e[5] * e[17] + e[17] * e[4] * e[7] + e[17] * e[3] * e[6] - 1.*e[5] * e[10] * e[1] - 1.*e[5] * e[16] * e[7] - 1.*e[5] * e[15] * e[6] - 1.*e[5] * e[9] * e[0] + e[2] * e[9] * e[3] + e[2] * e[0] * e[12] + e[2] * e[10] * e[4] + e[2] * e[1] * e[13] + .5000000000*ep2[2] * e[14] - .5000000000*e[14] * ep2[0] - .5000000000*e[14] * ep2[6] - .5000000000*e[14] * ep2[1] - .5000000000*e[14] * ep2[7] + 1.500000000*e[14] * ep2[5] + .5000000000*e[14] * ep2[4] + .5000000000*e[14] * ep2[3] + .5000000000*e[14] * ep2[8];
			A[31] = e[3] * e[27] * e[12] + e[3] * e[9] * e[30] + e[3] * e[29] * e[14] + e[3] * e[11] * e[32] + e[3] * e[28] * e[13] + e[3] * e[10] * e[31] + e[6] * e[27] * e[15] + e[6] * e[9] * e[33] + e[6] * e[29] * e[17] + e[6] * e[11] * e[35] + e[6] * e[28] * e[16] + e[6] * e[10] * e[34] + 3.*e[0] * e[27] * e[9] + e[0] * e[29] * e[11] + e[0] * e[28] * e[10] - 1.*e[9] * e[34] * e[7] - 1.*e[9] * e[32] * e[5] - 1.*e[9] * e[35] * e[8] + e[9] * e[29] * e[2] + e[9] * e[28] * e[1] - 1.*e[9] * e[31] * e[4] + e[12] * e[0] * e[30] + e[12] * e[28] * e[4] + e[12] * e[1] * e[31] + e[12] * e[29] * e[5] + e[12] * e[2] * e[32] + e[27] * e[11] * e[2] + e[27] * e[10] * e[1] - 1.*e[27] * e[13] * e[4] - 1.*e[27] * e[16] * e[7] - 1.*e[27] * e[14] * e[5] - 1.*e[27] * e[17] * e[8] + e[30] * e[10] * e[4] + e[30] * e[1] * e[13] + e[30] * e[11] * e[5] + e[30] * e[2] * e[14] + e[15] * e[0] * e[33] + e[15] * e[28] * e[7] + e[15] * e[1] * e[34] + e[15] * e[29] * e[8] + e[15] * e[2] * e[35] - 1.*e[0] * e[31] * e[13] - 1.*e[0] * e[32] * e[14] - 1.*e[0] * e[34] * e[16] - 1.*e[0] * e[35] * e[17] + e[33] * e[10] * e[7] + e[33] * e[1] * e[16] + e[33] * e[11] * e[8] + e[33] * e[2] * e[17];
			A[141] = .5000000000*ep2[30] * e[6] + .5000000000*e[6] * ep2[27] - .5000000000*e[6] * ep2[32] - .5000000000*e[6] * ep2[28] - .5000000000*e[6] * ep2[29] - .5000000000*e[6] * ep2[31] + 1.500000000*e[6] * ep2[33] + .5000000000*e[6] * ep2[34] + .5000000000*e[6] * ep2[35] + e[0] * e[27] * e[33] + e[0] * e[29] * e[35] + e[0] * e[28] * e[34] + e[3] * e[30] * e[33] + e[3] * e[32] * e[35] + e[3] * e[31] * e[34] + e[30] * e[31] * e[7] + e[30] * e[4] * e[34] + e[30] * e[32] * e[8] + e[30] * e[5] * e[35] + e[27] * e[28] * e[7] + e[27] * e[1] * e[34] + e[27] * e[29] * e[8] + e[27] * e[2] * e[35] + e[33] * e[34] * e[7] + e[33] * e[35] * e[8] - 1.*e[33] * e[32] * e[5] - 1.*e[33] * e[29] * e[2] - 1.*e[33] * e[28] * e[1] - 1.*e[33] * e[31] * e[4];
			A[30] = e[24] * e[20] * e[26] + e[21] * e[19] * e[22] - .5000000000*e[18] * ep2[22] - .5000000000*e[18] * ep2[25] + .5000000000*ep3[18] + .5000000000*e[18] * ep2[21] + e[21] * e[20] * e[23] + .5000000000*e[18] * ep2[20] + .5000000000*e[18] * ep2[19] + .5000000000*e[18] * ep2[24] + e[24] * e[19] * e[25] - .5000000000*e[18] * ep2[23] - .5000000000*e[18] * ep2[26];
			A[140] = .5000000000*e[33] * ep2[35] + .5000000000*ep3[33] + .5000000000*ep2[27] * e[33] + .5000000000*ep2[30] * e[33] - .5000000000*e[33] * ep2[29] + .5000000000*e[33] * ep2[34] - .5000000000*e[33] * ep2[32] - .5000000000*e[33] * ep2[28] + e[30] * e[32] * e[35] - .5000000000*e[33] * ep2[31] + e[27] * e[29] * e[35] + e[27] * e[28] * e[34] + e[30] * e[31] * e[34];
			A[29] = 1.500000000*e[27] * ep2[18] + .5000000000*e[27] * ep2[19] + .5000000000*e[27] * ep2[20] + .5000000000*e[27] * ep2[21] + .5000000000*e[27] * ep2[24] - .5000000000*e[27] * ep2[26] - .5000000000*e[27] * ep2[23] - .5000000000*e[27] * ep2[22] - .5000000000*e[27] * ep2[25] + e[33] * e[20] * e[26] - 1.*e[18] * e[35] * e[26] - 1.*e[18] * e[31] * e[22] - 1.*e[18] * e[32] * e[23] - 1.*e[18] * e[34] * e[25] + e[18] * e[28] * e[19] + e[18] * e[29] * e[20] + e[21] * e[18] * e[30] + e[21] * e[28] * e[22] + e[21] * e[19] * e[31] + e[21] * e[29] * e[23] + e[21] * e[20] * e[32] + e[30] * e[19] * e[22] + e[30] * e[20] * e[23] + e[24] * e[18] * e[33] + e[24] * e[28] * e[25] + e[24] * e[19] * e[34] + e[24] * e[29] * e[26] + e[24] * e[20] * e[35] + e[33] * e[19] * e[25];
			A[143] = e[9] * e[27] * e[33] + e[9] * e[29] * e[35] + e[9] * e[28] * e[34] + e[33] * e[35] * e[17] + e[33] * e[34] * e[16] + e[27] * e[29] * e[17] + e[27] * e[11] * e[35] + e[27] * e[28] * e[16] + e[27] * e[10] * e[34] + e[33] * e[30] * e[12] - 1.*e[33] * e[28] * e[10] - 1.*e[33] * e[31] * e[13] - 1.*e[33] * e[32] * e[14] - 1.*e[33] * e[29] * e[11] + e[30] * e[32] * e[17] + e[30] * e[14] * e[35] + e[30] * e[31] * e[16] + e[30] * e[13] * e[34] + e[12] * e[32] * e[35] + e[12] * e[31] * e[34] + .5000000000*e[15] * ep2[27] - .5000000000*e[15] * ep2[32] - .5000000000*e[15] * ep2[28] - .5000000000*e[15] * ep2[29] - .5000000000*e[15] * ep2[31] + 1.500000000*e[15] * ep2[33] + .5000000000*e[15] * ep2[30] + .5000000000*e[15] * ep2[34] + .5000000000*e[15] * ep2[35];
			A[28] = .5000000000*e[9] * ep2[12] - .5000000000*e[9] * ep2[16] + .5000000000*e[9] * ep2[10] - .5000000000*e[9] * ep2[17] - .5000000000*e[9] * ep2[13] + e[15] * e[10] * e[16] + e[12] * e[11] * e[14] + .5000000000*e[9] * ep2[11] + .5000000000*e[9] * ep2[15] - .5000000000*e[9] * ep2[14] + e[15] * e[11] * e[17] + .5000000000*ep3[9] + e[12] * e[10] * e[13];
			A[142] = e[18] * e[27] * e[33] + e[18] * e[29] * e[35] + e[18] * e[28] * e[34] + e[27] * e[28] * e[25] + e[27] * e[19] * e[34] + e[27] * e[29] * e[26] + e[27] * e[20] * e[35] + e[21] * e[30] * e[33] + e[21] * e[32] * e[35] + e[21] * e[31] * e[34] + e[30] * e[31] * e[25] + e[30] * e[22] * e[34] + e[30] * e[32] * e[26] + e[30] * e[23] * e[35] + e[33] * e[34] * e[25] + e[33] * e[35] * e[26] - 1.*e[33] * e[29] * e[20] - 1.*e[33] * e[31] * e[22] - 1.*e[33] * e[32] * e[23] - 1.*e[33] * e[28] * e[19] + .5000000000*ep2[27] * e[24] + .5000000000*ep2[30] * e[24] + 1.500000000*e[24] * ep2[33] + .5000000000*e[24] * ep2[35] + .5000000000*e[24] * ep2[34] - .5000000000*e[24] * ep2[32] - .5000000000*e[24] * ep2[28] - .5000000000*e[24] * ep2[29] - .5000000000*e[24] * ep2[31];
			A[36] = .5000000000*e[9] * ep2[21] + .5000000000*e[9] * ep2[24] + .5000000000*e[9] * ep2[19] + 1.500000000*e[9] * ep2[18] + .5000000000*e[9] * ep2[20] - .5000000000*e[9] * ep2[26] - .5000000000*e[9] * ep2[23] - .5000000000*e[9] * ep2[22] - .5000000000*e[9] * ep2[25] + e[21] * e[18] * e[12] + e[21] * e[20] * e[14] + e[21] * e[11] * e[23] + e[21] * e[19] * e[13] + e[21] * e[10] * e[22] + e[24] * e[18] * e[15] + e[24] * e[20] * e[17] + e[24] * e[11] * e[26] + e[24] * e[19] * e[16] + e[24] * e[10] * e[25] + e[15] * e[19] * e[25] + e[15] * e[20] * e[26] + e[12] * e[19] * e[22] + e[12] * e[20] * e[23] + e[18] * e[20] * e[11] + e[18] * e[19] * e[10] - 1.*e[18] * e[23] * e[14] - 1.*e[18] * e[25] * e[16] - 1.*e[18] * e[26] * e[17] - 1.*e[18] * e[22] * e[13];
			A[182] = .5000000000*ep2[29] * e[26] + .5000000000*ep2[32] * e[26] + .5000000000*e[26] * ep2[33] + 1.500000000*e[26] * ep2[35] + .5000000000*e[26] * ep2[34] - .5000000000*e[26] * ep2[27] - .5000000000*e[26] * ep2[28] - .5000000000*e[26] * ep2[31] - .5000000000*e[26] * ep2[30] + e[20] * e[27] * e[33] + e[20] * e[29] * e[35] + e[20] * e[28] * e[34] + e[29] * e[27] * e[24] + e[29] * e[18] * e[33] + e[29] * e[28] * e[25] + e[29] * e[19] * e[34] + e[23] * e[30] * e[33] + e[23] * e[32] * e[35] + e[23] * e[31] * e[34] + e[32] * e[30] * e[24] + e[32] * e[21] * e[33] + e[32] * e[31] * e[25] + e[32] * e[22] * e[34] + e[35] * e[33] * e[24] + e[35] * e[34] * e[25] - 1.*e[35] * e[27] * e[18] - 1.*e[35] * e[30] * e[21] - 1.*e[35] * e[31] * e[22] - 1.*e[35] * e[28] * e[19];
			A[37] = e[12] * e[19] * e[31] + e[12] * e[29] * e[23] + e[12] * e[20] * e[32] + 3.*e[9] * e[27] * e[18] + e[9] * e[28] * e[19] + e[9] * e[29] * e[20] + e[21] * e[9] * e[30] + e[21] * e[29] * e[14] + e[21] * e[11] * e[32] + e[21] * e[28] * e[13] + e[21] * e[10] * e[31] + e[30] * e[20] * e[14] + e[30] * e[11] * e[23] + e[30] * e[19] * e[13] + e[30] * e[10] * e[22] + e[9] * e[33] * e[24] - 1.*e[9] * e[35] * e[26] - 1.*e[9] * e[31] * e[22] - 1.*e[9] * e[32] * e[23] - 1.*e[9] * e[34] * e[25] + e[18] * e[29] * e[11] + e[18] * e[28] * e[10] + e[27] * e[20] * e[11] + e[27] * e[19] * e[10] + e[15] * e[27] * e[24] + e[15] * e[18] * e[33] + e[15] * e[28] * e[25] + e[15] * e[19] * e[34] + e[15] * e[29] * e[26] + e[15] * e[20] * e[35] - 1.*e[18] * e[31] * e[13] - 1.*e[18] * e[32] * e[14] - 1.*e[18] * e[34] * e[16] - 1.*e[18] * e[35] * e[17] - 1.*e[27] * e[23] * e[14] - 1.*e[27] * e[25] * e[16] - 1.*e[27] * e[26] * e[17] - 1.*e[27] * e[22] * e[13] + e[24] * e[29] * e[17] + e[24] * e[11] * e[35] + e[24] * e[28] * e[16] + e[24] * e[10] * e[34] + e[33] * e[20] * e[17] + e[33] * e[11] * e[26] + e[33] * e[19] * e[16] + e[33] * e[10] * e[25] + e[12] * e[27] * e[21] + e[12] * e[18] * e[30] + e[12] * e[28] * e[22];
			A[183] = -.5000000000*e[17] * ep2[27] + .5000000000*e[17] * ep2[32] - .5000000000*e[17] * ep2[28] + .5000000000*e[17] * ep2[29] - .5000000000*e[17] * ep2[31] + .5000000000*e[17] * ep2[33] - .5000000000*e[17] * ep2[30] + .5000000000*e[17] * ep2[34] + 1.500000000*e[17] * ep2[35] + e[32] * e[30] * e[15] + e[32] * e[12] * e[33] + e[32] * e[31] * e[16] + e[32] * e[13] * e[34] + e[14] * e[30] * e[33] + e[14] * e[31] * e[34] + e[11] * e[27] * e[33] + e[11] * e[29] * e[35] + e[11] * e[28] * e[34] + e[35] * e[33] * e[15] + e[35] * e[34] * e[16] + e[29] * e[27] * e[15] + e[29] * e[9] * e[33] + e[29] * e[28] * e[16] + e[29] * e[10] * e[34] - 1.*e[35] * e[27] * e[9] - 1.*e[35] * e[30] * e[12] - 1.*e[35] * e[28] * e[10] - 1.*e[35] * e[31] * e[13] + e[35] * e[32] * e[14];
			A[38] = .5000000000*e[9] * ep2[1] + 1.500000000*e[9] * ep2[0] + .5000000000*e[9] * ep2[2] + .5000000000*e[9] * ep2[3] + .5000000000*e[9] * ep2[6] - .5000000000*e[9] * ep2[4] - .5000000000*e[9] * ep2[5] - .5000000000*e[9] * ep2[7] - .5000000000*e[9] * ep2[8] + e[6] * e[0] * e[15] + e[6] * e[10] * e[7] + e[6] * e[1] * e[16] + e[6] * e[11] * e[8] + e[6] * e[2] * e[17] + e[15] * e[1] * e[7] + e[15] * e[2] * e[8] + e[0] * e[11] * e[2] + e[0] * e[10] * e[1] - 1.*e[0] * e[13] * e[4] - 1.*e[0] * e[16] * e[7] - 1.*e[0] * e[14] * e[5] - 1.*e[0] * e[17] * e[8] + e[3] * e[0] * e[12] + e[3] * e[10] * e[4] + e[3] * e[1] * e[13] + e[3] * e[11] * e[5] + e[3] * e[2] * e[14] + e[12] * e[1] * e[4] + e[12] * e[2] * e[5];
			A[180] = .5000000000*e[35] * ep2[33] + .5000000000*e[35] * ep2[34] - .5000000000*e[35] * ep2[27] - .5000000000*e[35] * ep2[28] - .5000000000*e[35] * ep2[31] - .5000000000*e[35] * ep2[30] + e[32] * e[31] * e[34] + .5000000000*ep2[29] * e[35] + .5000000000*ep2[32] * e[35] + e[29] * e[28] * e[34] + e[32] * e[30] * e[33] + .5000000000*ep3[35] + e[29] * e[27] * e[33];
			A[39] = .5000000000*e[0] * ep2[19] + .5000000000*e[0] * ep2[20] + .5000000000*e[0] * ep2[24] - .5000000000*e[0] * ep2[26] - .5000000000*e[0] * ep2[23] - .5000000000*e[0] * ep2[22] - .5000000000*e[0] * ep2[25] + 1.500000000*ep2[18] * e[0] + .5000000000*e[0] * ep2[21] + e[18] * e[19] * e[1] + e[18] * e[20] * e[2] + e[21] * e[18] * e[3] + e[21] * e[19] * e[4] + e[21] * e[1] * e[22] + e[21] * e[20] * e[5] + e[21] * e[2] * e[23] - 1.*e[18] * e[26] * e[8] - 1.*e[18] * e[22] * e[4] - 1.*e[18] * e[25] * e[7] - 1.*e[18] * e[23] * e[5] + e[18] * e[24] * e[6] + e[3] * e[19] * e[22] + e[3] * e[20] * e[23] + e[24] * e[19] * e[7] + e[24] * e[1] * e[25] + e[24] * e[20] * e[8] + e[24] * e[2] * e[26] + e[6] * e[19] * e[25] + e[6] * e[20] * e[26];
			A[181] = .5000000000*ep2[32] * e[8] - .5000000000*e[8] * ep2[27] - .5000000000*e[8] * ep2[28] + .5000000000*e[8] * ep2[29] - .5000000000*e[8] * ep2[31] + .5000000000*e[8] * ep2[33] - .5000000000*e[8] * ep2[30] + .5000000000*e[8] * ep2[34] + 1.500000000*e[8] * ep2[35] + e[2] * e[27] * e[33] + e[2] * e[29] * e[35] + e[2] * e[28] * e[34] + e[5] * e[30] * e[33] + e[5] * e[32] * e[35] + e[5] * e[31] * e[34] + e[32] * e[30] * e[6] + e[32] * e[3] * e[33] + e[32] * e[31] * e[7] + e[32] * e[4] * e[34] + e[29] * e[27] * e[6] + e[29] * e[0] * e[33] + e[29] * e[28] * e[7] + e[29] * e[1] * e[34] + e[35] * e[33] * e[6] + e[35] * e[34] * e[7] - 1.*e[35] * e[27] * e[0] - 1.*e[35] * e[30] * e[3] - 1.*e[35] * e[28] * e[1] - 1.*e[35] * e[31] * e[4];
			A[32] = -.5000000000*e[18] * ep2[4] + 1.500000000*e[18] * ep2[0] + .5000000000*e[18] * ep2[6] - .5000000000*e[18] * ep2[5] + .5000000000*e[18] * ep2[1] - .5000000000*e[18] * ep2[7] + .5000000000*e[18] * ep2[3] + .5000000000*e[18] * ep2[2] - .5000000000*e[18] * ep2[8] + e[3] * e[0] * e[21] + e[3] * e[19] * e[4] + e[3] * e[1] * e[22] + e[3] * e[20] * e[5] + e[3] * e[2] * e[23] + e[21] * e[1] * e[4] + e[21] * e[2] * e[5] + e[6] * e[0] * e[24] + e[6] * e[19] * e[7] + e[6] * e[1] * e[25] + e[6] * e[20] * e[8] + e[6] * e[2] * e[26] + e[24] * e[1] * e[7] + e[24] * e[2] * e[8] + e[0] * e[19] * e[1] + e[0] * e[20] * e[2] - 1.*e[0] * e[26] * e[8] - 1.*e[0] * e[22] * e[4] - 1.*e[0] * e[25] * e[7] - 1.*e[0] * e[23] * e[5];
			A[178] = e[10] * e[1] * e[7] + e[10] * e[0] * e[6] + e[10] * e[2] * e[8] + e[4] * e[12] * e[6] + e[4] * e[3] * e[15] + e[4] * e[13] * e[7] + e[4] * e[14] * e[8] + e[4] * e[5] * e[17] + e[13] * e[3] * e[6] + e[13] * e[5] * e[8] + e[7] * e[15] * e[6] + e[7] * e[17] * e[8] - 1.*e[7] * e[11] * e[2] - 1.*e[7] * e[9] * e[0] - 1.*e[7] * e[14] * e[5] - 1.*e[7] * e[12] * e[3] + e[1] * e[9] * e[6] + e[1] * e[0] * e[15] + e[1] * e[11] * e[8] + e[1] * e[2] * e[17] + 1.500000000*e[16] * ep2[7] + .5000000000*e[16] * ep2[6] + .5000000000*e[16] * ep2[8] + .5000000000*ep2[1] * e[16] - .5000000000*e[16] * ep2[0] - .5000000000*e[16] * ep2[5] - .5000000000*e[16] * ep2[3] - .5000000000*e[16] * ep2[2] + .5000000000*ep2[4] * e[16];
			A[33] = e[0] * e[30] * e[21] - 1.*e[0] * e[35] * e[26] - 1.*e[0] * e[31] * e[22] - 1.*e[0] * e[32] * e[23] - 1.*e[0] * e[34] * e[25] - 1.*e[18] * e[34] * e[7] - 1.*e[18] * e[32] * e[5] - 1.*e[18] * e[35] * e[8] - 1.*e[18] * e[31] * e[4] - 1.*e[27] * e[26] * e[8] - 1.*e[27] * e[22] * e[4] - 1.*e[27] * e[25] * e[7] - 1.*e[27] * e[23] * e[5] + e[6] * e[28] * e[25] + e[6] * e[19] * e[34] + e[6] * e[29] * e[26] + e[6] * e[20] * e[35] + e[21] * e[28] * e[4] + e[21] * e[1] * e[31] + e[21] * e[29] * e[5] + e[21] * e[2] * e[32] + e[30] * e[19] * e[4] + e[30] * e[1] * e[22] + e[30] * e[20] * e[5] + e[30] * e[2] * e[23] + e[24] * e[27] * e[6] + e[24] * e[0] * e[33] + e[24] * e[28] * e[7] + e[24] * e[1] * e[34] + e[24] * e[29] * e[8] + e[24] * e[2] * e[35] + e[33] * e[18] * e[6] + e[33] * e[19] * e[7] + e[33] * e[1] * e[25] + e[33] * e[20] * e[8] + e[33] * e[2] * e[26] + 3.*e[0] * e[27] * e[18] + e[0] * e[28] * e[19] + e[0] * e[29] * e[20] + e[18] * e[28] * e[1] + e[18] * e[29] * e[2] + e[27] * e[19] * e[1] + e[27] * e[20] * e[2] + e[3] * e[27] * e[21] + e[3] * e[18] * e[30] + e[3] * e[28] * e[22] + e[3] * e[19] * e[31] + e[3] * e[29] * e[23] + e[3] * e[20] * e[32];
			A[179] = e[19] * e[18] * e[6] + e[19] * e[0] * e[24] + e[19] * e[1] * e[25] + e[19] * e[20] * e[8] + e[19] * e[2] * e[26] + e[22] * e[21] * e[6] + e[22] * e[3] * e[24] + e[22] * e[4] * e[25] + e[22] * e[23] * e[8] + e[22] * e[5] * e[26] - 1.*e[25] * e[21] * e[3] + e[25] * e[26] * e[8] - 1.*e[25] * e[20] * e[2] - 1.*e[25] * e[18] * e[0] - 1.*e[25] * e[23] * e[5] + e[25] * e[24] * e[6] + e[1] * e[18] * e[24] + e[1] * e[20] * e[26] + e[4] * e[21] * e[24] + e[4] * e[23] * e[26] + .5000000000*ep2[19] * e[7] + .5000000000*ep2[22] * e[7] + 1.500000000*ep2[25] * e[7] + .5000000000*e[7] * ep2[26] - .5000000000*e[7] * ep2[18] - .5000000000*e[7] * ep2[23] - .5000000000*e[7] * ep2[20] + .5000000000*e[7] * ep2[24] - .5000000000*e[7] * ep2[21];
			A[34] = .5000000000*e[18] * ep2[11] + 1.500000000*e[18] * ep2[9] + .5000000000*e[18] * ep2[10] + .5000000000*e[18] * ep2[12] + .5000000000*e[18] * ep2[15] - .5000000000*e[18] * ep2[16] - .5000000000*e[18] * ep2[17] - .5000000000*e[18] * ep2[14] - .5000000000*e[18] * ep2[13] + e[12] * e[9] * e[21] + e[12] * e[20] * e[14] + e[12] * e[11] * e[23] + e[12] * e[19] * e[13] + e[12] * e[10] * e[22] + e[21] * e[11] * e[14] + e[21] * e[10] * e[13] + e[15] * e[9] * e[24] + e[15] * e[20] * e[17] + e[15] * e[11] * e[26] + e[15] * e[19] * e[16] + e[15] * e[10] * e[25] + e[24] * e[11] * e[17] + e[24] * e[10] * e[16] - 1.*e[9] * e[23] * e[14] - 1.*e[9] * e[25] * e[16] - 1.*e[9] * e[26] * e[17] + e[9] * e[20] * e[11] + e[9] * e[19] * e[10] - 1.*e[9] * e[22] * e[13];
			A[176] = e[13] * e[21] * e[24] + e[13] * e[23] * e[26] + e[19] * e[18] * e[15] + e[19] * e[9] * e[24] + e[19] * e[20] * e[17] + e[19] * e[11] * e[26] - 1.*e[25] * e[23] * e[14] - 1.*e[25] * e[20] * e[11] - 1.*e[25] * e[18] * e[9] - 1.*e[25] * e[21] * e[12] + e[22] * e[21] * e[15] + e[22] * e[12] * e[24] + e[22] * e[23] * e[17] + e[22] * e[14] * e[26] + e[22] * e[13] * e[25] + e[25] * e[24] * e[15] + e[25] * e[26] * e[17] + e[10] * e[19] * e[25] + e[10] * e[18] * e[24] + e[10] * e[20] * e[26] - .5000000000*e[16] * ep2[18] - .5000000000*e[16] * ep2[23] + .5000000000*e[16] * ep2[19] - .5000000000*e[16] * ep2[20] - .5000000000*e[16] * ep2[21] + .5000000000*ep2[22] * e[16] + 1.500000000*ep2[25] * e[16] + .5000000000*e[16] * ep2[24] + .5000000000*e[16] * ep2[26];
			A[35] = .5000000000*e[0] * ep2[12] + .5000000000*e[0] * ep2[15] + .5000000000*e[0] * ep2[11] + 1.500000000*e[0] * ep2[9] + .5000000000*e[0] * ep2[10] - .5000000000*e[0] * ep2[16] - .5000000000*e[0] * ep2[17] - .5000000000*e[0] * ep2[14] - .5000000000*e[0] * ep2[13] + e[12] * e[9] * e[3] + e[12] * e[10] * e[4] + e[12] * e[1] * e[13] + e[12] * e[11] * e[5] + e[12] * e[2] * e[14] + e[15] * e[9] * e[6] + e[15] * e[10] * e[7] + e[15] * e[1] * e[16] + e[15] * e[11] * e[8] + e[15] * e[2] * e[17] + e[6] * e[11] * e[17] + e[6] * e[10] * e[16] + e[3] * e[11] * e[14] + e[3] * e[10] * e[13] + e[9] * e[10] * e[1] + e[9] * e[11] * e[2] - 1.*e[9] * e[13] * e[4] - 1.*e[9] * e[16] * e[7] - 1.*e[9] * e[14] * e[5] - 1.*e[9] * e[17] * e[8];
			A[177] = e[19] * e[11] * e[35] + e[28] * e[18] * e[15] + e[28] * e[9] * e[24] + e[28] * e[20] * e[17] + e[28] * e[11] * e[26] - 1.*e[25] * e[27] * e[9] - 1.*e[25] * e[30] * e[12] - 1.*e[25] * e[32] * e[14] + e[25] * e[33] * e[15] + e[25] * e[35] * e[17] - 1.*e[25] * e[29] * e[11] - 1.*e[34] * e[23] * e[14] + e[34] * e[24] * e[15] + e[34] * e[26] * e[17] - 1.*e[34] * e[20] * e[11] - 1.*e[34] * e[18] * e[9] - 1.*e[34] * e[21] * e[12] + e[13] * e[30] * e[24] + e[13] * e[21] * e[33] + e[13] * e[31] * e[25] + e[13] * e[22] * e[34] + e[13] * e[32] * e[26] + e[13] * e[23] * e[35] + e[10] * e[27] * e[24] + e[10] * e[18] * e[33] + e[10] * e[28] * e[25] + e[10] * e[19] * e[34] + e[10] * e[29] * e[26] + e[10] * e[20] * e[35] + e[22] * e[30] * e[15] + e[22] * e[12] * e[33] + e[22] * e[32] * e[17] + e[22] * e[14] * e[35] + e[22] * e[31] * e[16] + e[31] * e[21] * e[15] + e[31] * e[12] * e[24] + e[31] * e[23] * e[17] + e[31] * e[14] * e[26] - 1.*e[16] * e[27] * e[18] + e[16] * e[33] * e[24] - 1.*e[16] * e[30] * e[21] - 1.*e[16] * e[29] * e[20] + e[16] * e[35] * e[26] - 1.*e[16] * e[32] * e[23] + e[16] * e[28] * e[19] + 3.*e[16] * e[34] * e[25] + e[19] * e[27] * e[15] + e[19] * e[9] * e[33] + e[19] * e[29] * e[17];
			A[45] = e[4] * e[27] * e[3] + e[4] * e[0] * e[30] + e[4] * e[29] * e[5] + e[4] * e[2] * e[32] + e[31] * e[0] * e[3] + e[31] * e[2] * e[5] + e[7] * e[27] * e[6] + e[7] * e[0] * e[33] + e[7] * e[29] * e[8] + e[7] * e[2] * e[35] + e[34] * e[0] * e[6] + e[34] * e[2] * e[8] + e[1] * e[27] * e[0] + e[1] * e[29] * e[2] + e[1] * e[34] * e[7] - 1.*e[1] * e[32] * e[5] - 1.*e[1] * e[33] * e[6] - 1.*e[1] * e[30] * e[3] - 1.*e[1] * e[35] * e[8] + e[1] * e[31] * e[4] + 1.500000000*e[28] * ep2[1] + .5000000000*e[28] * ep2[4] + .5000000000*e[28] * ep2[0] - .5000000000*e[28] * ep2[6] - .5000000000*e[28] * ep2[5] + .5000000000*e[28] * ep2[7] - .5000000000*e[28] * ep2[3] + .5000000000*e[28] * ep2[2] - .5000000000*e[28] * ep2[8];
			A[191] = -1.*e[35] * e[10] * e[1] - 1.*e[35] * e[13] * e[4] + e[35] * e[16] * e[7] + e[35] * e[15] * e[6] - 1.*e[35] * e[9] * e[0] - 1.*e[35] * e[12] * e[3] + e[32] * e[12] * e[6] + e[32] * e[3] * e[15] + e[32] * e[13] * e[7] + e[32] * e[4] * e[16] - 1.*e[8] * e[27] * e[9] - 1.*e[8] * e[30] * e[12] - 1.*e[8] * e[28] * e[10] - 1.*e[8] * e[31] * e[13] + e[8] * e[29] * e[11] + e[11] * e[27] * e[6] + e[11] * e[0] * e[33] + e[11] * e[28] * e[7] + e[11] * e[1] * e[34] + e[29] * e[9] * e[6] + e[29] * e[0] * e[15] + e[29] * e[10] * e[7] + e[29] * e[1] * e[16] + e[5] * e[30] * e[15] + e[5] * e[12] * e[33] + e[5] * e[32] * e[17] + e[5] * e[14] * e[35] + e[5] * e[31] * e[16] + e[5] * e[13] * e[34] + e[8] * e[33] * e[15] + 3.*e[8] * e[35] * e[17] + e[8] * e[34] * e[16] + e[2] * e[27] * e[15] + e[2] * e[9] * e[33] + e[2] * e[29] * e[17] + e[2] * e[11] * e[35] + e[2] * e[28] * e[16] + e[2] * e[10] * e[34] - 1.*e[17] * e[27] * e[0] + e[17] * e[34] * e[7] + e[17] * e[33] * e[6] - 1.*e[17] * e[30] * e[3] - 1.*e[17] * e[28] * e[1] - 1.*e[17] * e[31] * e[4] + e[14] * e[30] * e[6] + e[14] * e[3] * e[33] + e[14] * e[31] * e[7] + e[14] * e[4] * e[34] + e[14] * e[32] * e[8];
			A[44] = e[19] * e[11] * e[2] + e[4] * e[18] * e[12] + e[4] * e[9] * e[21] + e[4] * e[20] * e[14] + e[4] * e[11] * e[23] + e[4] * e[19] * e[13] + e[4] * e[10] * e[22] + e[7] * e[18] * e[15] + e[7] * e[9] * e[24] + e[7] * e[20] * e[17] + e[7] * e[11] * e[26] + e[7] * e[19] * e[16] + e[7] * e[10] * e[25] + e[1] * e[18] * e[9] + e[1] * e[20] * e[11] - 1.*e[10] * e[21] * e[3] - 1.*e[10] * e[26] * e[8] - 1.*e[10] * e[23] * e[5] - 1.*e[10] * e[24] * e[6] + e[13] * e[18] * e[3] + e[13] * e[0] * e[21] + e[13] * e[1] * e[22] + e[13] * e[20] * e[5] + e[13] * e[2] * e[23] - 1.*e[19] * e[15] * e[6] - 1.*e[19] * e[14] * e[5] - 1.*e[19] * e[12] * e[3] - 1.*e[19] * e[17] * e[8] + e[22] * e[9] * e[3] + e[22] * e[0] * e[12] + e[22] * e[11] * e[5] + e[22] * e[2] * e[14] + e[16] * e[18] * e[6] + e[16] * e[0] * e[24] + e[16] * e[1] * e[25] + e[16] * e[20] * e[8] + e[16] * e[2] * e[26] - 1.*e[1] * e[23] * e[14] - 1.*e[1] * e[24] * e[15] - 1.*e[1] * e[26] * e[17] - 1.*e[1] * e[21] * e[12] + e[25] * e[9] * e[6] + e[25] * e[0] * e[15] + e[25] * e[11] * e[8] + e[25] * e[2] * e[17] + e[10] * e[18] * e[0] + 3.*e[10] * e[19] * e[1] + e[10] * e[20] * e[2] + e[19] * e[9] * e[0];
			A[190] = .5000000000*ep2[23] * e[26] + .5000000000*e[26] * ep2[25] + .5000000000*ep2[20] * e[26] - .5000000000*e[26] * ep2[18] + .5000000000*ep3[26] + .5000000000*e[26] * ep2[24] + e[20] * e[19] * e[25] - .5000000000*e[26] * ep2[19] - .5000000000*e[26] * ep2[21] + e[20] * e[18] * e[24] - .5000000000*e[26] * ep2[22] + e[23] * e[21] * e[24] + e[23] * e[22] * e[25];
			A[47] = e[16] * e[9] * e[33] + e[16] * e[29] * e[17] + e[16] * e[11] * e[35] + e[16] * e[10] * e[34] + e[34] * e[11] * e[17] + e[34] * e[9] * e[15] - 1.*e[10] * e[30] * e[12] - 1.*e[10] * e[32] * e[14] - 1.*e[10] * e[33] * e[15] - 1.*e[10] * e[35] * e[17] + e[10] * e[27] * e[9] + e[10] * e[29] * e[11] + e[13] * e[27] * e[12] + e[13] * e[9] * e[30] + e[13] * e[29] * e[14] + e[13] * e[11] * e[32] + e[13] * e[10] * e[31] + e[31] * e[11] * e[14] + e[31] * e[9] * e[12] + e[16] * e[27] * e[15] + 1.500000000*e[28] * ep2[10] + .5000000000*e[28] * ep2[16] + .5000000000*e[28] * ep2[9] + .5000000000*e[28] * ep2[11] - .5000000000*e[28] * ep2[12] - .5000000000*e[28] * ep2[15] - .5000000000*e[28] * ep2[17] - .5000000000*e[28] * ep2[14] + .5000000000*e[28] * ep2[13];
			A[189] = .5000000000*ep2[20] * e[35] + .5000000000*ep2[23] * e[35] + 1.500000000*e[35] * ep2[26] + .5000000000*e[35] * ep2[25] + .5000000000*e[35] * ep2[24] - .5000000000*e[35] * ep2[18] - .5000000000*e[35] * ep2[19] - .5000000000*e[35] * ep2[22] - .5000000000*e[35] * ep2[21] + e[20] * e[27] * e[24] + e[20] * e[18] * e[33] + e[20] * e[28] * e[25] + e[20] * e[19] * e[34] + e[20] * e[29] * e[26] + e[29] * e[19] * e[25] + e[29] * e[18] * e[24] + e[23] * e[30] * e[24] + e[23] * e[21] * e[33] + e[23] * e[31] * e[25] + e[23] * e[22] * e[34] + e[23] * e[32] * e[26] + e[32] * e[22] * e[25] + e[32] * e[21] * e[24] + e[26] * e[33] * e[24] + e[26] * e[34] * e[25] - 1.*e[26] * e[27] * e[18] - 1.*e[26] * e[30] * e[21] - 1.*e[26] * e[31] * e[22] - 1.*e[26] * e[28] * e[19];
			A[46] = e[4] * e[2] * e[5] + .5000000000*e[1] * ep2[0] - .5000000000*e[1] * ep2[6] + e[7] * e[0] * e[6] + .5000000000*e[1] * ep2[7] + .5000000000*e[1] * ep2[4] - .5000000000*e[1] * ep2[8] + .5000000000*e[1] * ep2[2] - .5000000000*e[1] * ep2[3] + .5000000000*ep3[1] + e[7] * e[2] * e[8] - .5000000000*e[1] * ep2[5] + e[4] * e[0] * e[3];
			A[188] = -.5000000000*e[17] * ep2[13] - .5000000000*e[17] * ep2[9] + .5000000000*e[17] * ep2[16] + .5000000000*e[17] * ep2[15] + .5000000000*ep3[17] - .5000000000*e[17] * ep2[10] + e[14] * e[13] * e[16] + e[14] * e[12] * e[15] + .5000000000*ep2[14] * e[17] + e[11] * e[10] * e[16] - .5000000000*e[17] * ep2[12] + .5000000000*ep2[11] * e[17] + e[11] * e[9] * e[15];
			A[41] = e[4] * e[27] * e[30] + e[4] * e[29] * e[32] + e[4] * e[28] * e[31] + e[31] * e[27] * e[3] + e[31] * e[0] * e[30] + e[31] * e[29] * e[5] + e[31] * e[2] * e[32] + e[7] * e[27] * e[33] + e[7] * e[29] * e[35] + e[7] * e[28] * e[34] + e[28] * e[27] * e[0] + e[28] * e[29] * e[2] + e[34] * e[27] * e[6] + e[34] * e[0] * e[33] + e[34] * e[29] * e[8] + e[34] * e[2] * e[35] - 1.*e[28] * e[32] * e[5] - 1.*e[28] * e[33] * e[6] - 1.*e[28] * e[30] * e[3] - 1.*e[28] * e[35] * e[8] + .5000000000*e[1] * ep2[27] + .5000000000*e[1] * ep2[29] + 1.500000000*e[1] * ep2[28] + .5000000000*e[1] * ep2[31] - .5000000000*e[1] * ep2[32] - .5000000000*e[1] * ep2[33] - .5000000000*e[1] * ep2[30] + .5000000000*e[1] * ep2[34] - .5000000000*e[1] * ep2[35];
			A[187] = .5000000000*ep2[11] * e[35] + .5000000000*e[35] * ep2[16] - .5000000000*e[35] * ep2[9] - .5000000000*e[35] * ep2[12] + .5000000000*e[35] * ep2[15] + 1.500000000*e[35] * ep2[17] - .5000000000*e[35] * ep2[10] + .5000000000*e[35] * ep2[14] - .5000000000*e[35] * ep2[13] + e[11] * e[27] * e[15] + e[11] * e[9] * e[33] + e[11] * e[29] * e[17] + e[11] * e[28] * e[16] + e[11] * e[10] * e[34] + e[29] * e[9] * e[15] + e[29] * e[10] * e[16] + e[14] * e[30] * e[15] + e[14] * e[12] * e[33] + e[14] * e[32] * e[17] + e[14] * e[31] * e[16] + e[14] * e[13] * e[34] + e[32] * e[12] * e[15] + e[32] * e[13] * e[16] + e[17] * e[33] * e[15] + e[17] * e[34] * e[16] - 1.*e[17] * e[27] * e[9] - 1.*e[17] * e[30] * e[12] - 1.*e[17] * e[28] * e[10] - 1.*e[17] * e[31] * e[13];
			A[40] = e[34] * e[27] * e[33] + e[34] * e[29] * e[35] - .5000000000*e[28] * ep2[30] - .5000000000*e[28] * ep2[35] + .5000000000*ep3[28] + .5000000000*e[28] * ep2[27] + .5000000000*e[28] * ep2[29] + e[31] * e[27] * e[30] + e[31] * e[29] * e[32] - .5000000000*e[28] * ep2[32] - .5000000000*e[28] * ep2[33] + .5000000000*e[28] * ep2[31] + .5000000000*e[28] * ep2[34];
			A[186] = .5000000000*ep2[5] * e[8] + e[2] * e[0] * e[6] + .5000000000*ep2[2] * e[8] + .5000000000*ep3[8] - .5000000000*e[8] * ep2[0] + e[5] * e[4] * e[7] + e[5] * e[3] * e[6] + .5000000000*e[8] * ep2[7] + e[2] * e[1] * e[7] - .5000000000*e[8] * ep2[1] - .5000000000*e[8] * ep2[4] - .5000000000*e[8] * ep2[3] + .5000000000*e[8] * ep2[6];
			A[43] = e[28] * e[27] * e[9] + e[28] * e[29] * e[11] - 1.*e[28] * e[30] * e[12] + e[28] * e[31] * e[13] - 1.*e[28] * e[32] * e[14] - 1.*e[28] * e[33] * e[15] - 1.*e[28] * e[35] * e[17] + e[31] * e[27] * e[12] + e[31] * e[9] * e[30] + e[31] * e[29] * e[14] + e[31] * e[11] * e[32] + e[13] * e[27] * e[30] + e[13] * e[29] * e[32] + e[16] * e[27] * e[33] + e[16] * e[29] * e[35] + e[34] * e[27] * e[15] + e[34] * e[9] * e[33] + e[34] * e[29] * e[17] + e[34] * e[11] * e[35] + e[34] * e[28] * e[16] + .5000000000*e[10] * ep2[27] + .5000000000*e[10] * ep2[29] + 1.500000000*e[10] * ep2[28] - .5000000000*e[10] * ep2[32] + .5000000000*e[10] * ep2[31] - .5000000000*e[10] * ep2[33] - .5000000000*e[10] * ep2[30] + .5000000000*e[10] * ep2[34] - .5000000000*e[10] * ep2[35];
			A[185] = -.5000000000*e[35] * ep2[1] + .5000000000*e[35] * ep2[7] - .5000000000*e[35] * ep2[3] + .5000000000*ep2[2] * e[35] + 1.500000000*e[35] * ep2[8] - .5000000000*e[35] * ep2[4] - .5000000000*e[35] * ep2[0] + .5000000000*e[35] * ep2[6] + .5000000000*e[35] * ep2[5] + e[2] * e[27] * e[6] + e[2] * e[0] * e[33] + e[2] * e[28] * e[7] + e[2] * e[1] * e[34] + e[2] * e[29] * e[8] - 1.*e[8] * e[27] * e[0] + e[8] * e[34] * e[7] + e[8] * e[32] * e[5] + e[8] * e[33] * e[6] - 1.*e[8] * e[30] * e[3] - 1.*e[8] * e[28] * e[1] - 1.*e[8] * e[31] * e[4] + e[29] * e[1] * e[7] + e[29] * e[0] * e[6] + e[5] * e[30] * e[6] + e[5] * e[3] * e[33] + e[5] * e[31] * e[7] + e[5] * e[4] * e[34] + e[32] * e[4] * e[7] + e[32] * e[3] * e[6];
			A[42] = e[28] * e[27] * e[18] + e[28] * e[29] * e[20] + e[22] * e[27] * e[30] + e[22] * e[29] * e[32] + e[22] * e[28] * e[31] + e[31] * e[27] * e[21] + e[31] * e[18] * e[30] + e[31] * e[29] * e[23] + e[31] * e[20] * e[32] + e[25] * e[27] * e[33] + e[25] * e[29] * e[35] + e[25] * e[28] * e[34] + e[34] * e[27] * e[24] + e[34] * e[18] * e[33] + e[34] * e[29] * e[26] + e[34] * e[20] * e[35] - 1.*e[28] * e[33] * e[24] - 1.*e[28] * e[30] * e[21] - 1.*e[28] * e[35] * e[26] - 1.*e[28] * e[32] * e[23] - .5000000000*e[19] * ep2[33] - .5000000000*e[19] * ep2[30] - .5000000000*e[19] * ep2[35] + .5000000000*e[19] * ep2[27] + .5000000000*e[19] * ep2[29] + 1.500000000*e[19] * ep2[28] + .5000000000*e[19] * ep2[31] + .5000000000*e[19] * ep2[34] - .5000000000*e[19] * ep2[32];
			A[184] = e[23] * e[3] * e[15] - 1.*e[17] * e[19] * e[1] - 1.*e[17] * e[22] * e[4] - 1.*e[17] * e[18] * e[0] + e[17] * e[25] * e[7] + e[17] * e[24] * e[6] + e[14] * e[21] * e[6] + e[14] * e[3] * e[24] + e[14] * e[22] * e[7] + e[14] * e[4] * e[25] + e[14] * e[23] * e[8] - 1.*e[26] * e[10] * e[1] - 1.*e[26] * e[13] * e[4] + e[26] * e[16] * e[7] + e[26] * e[15] * e[6] - 1.*e[26] * e[9] * e[0] - 1.*e[26] * e[12] * e[3] + e[23] * e[12] * e[6] + e[11] * e[18] * e[6] + e[11] * e[0] * e[24] + e[11] * e[19] * e[7] + e[11] * e[1] * e[25] + e[11] * e[20] * e[8] + e[11] * e[2] * e[26] + e[20] * e[9] * e[6] + e[20] * e[0] * e[15] + e[20] * e[10] * e[7] + e[20] * e[1] * e[16] + e[20] * e[2] * e[17] + e[5] * e[21] * e[15] + e[5] * e[12] * e[24] + e[5] * e[23] * e[17] + e[5] * e[14] * e[26] + e[5] * e[22] * e[16] + e[5] * e[13] * e[25] + e[8] * e[24] * e[15] + 3.*e[8] * e[26] * e[17] + e[8] * e[25] * e[16] + e[2] * e[18] * e[15] + e[2] * e[9] * e[24] + e[2] * e[19] * e[16] + e[2] * e[10] * e[25] - 1.*e[17] * e[21] * e[3] + e[23] * e[4] * e[16] + e[23] * e[13] * e[7] - 1.*e[8] * e[18] * e[9] - 1.*e[8] * e[21] * e[12] - 1.*e[8] * e[19] * e[10] - 1.*e[8] * e[22] * e[13];
			A[54] = e[13] * e[18] * e[12] + e[13] * e[9] * e[21] + e[13] * e[20] * e[14] + e[13] * e[11] * e[23] + e[13] * e[10] * e[22] + e[22] * e[11] * e[14] + e[22] * e[9] * e[12] + e[16] * e[18] * e[15] + e[16] * e[9] * e[24] + e[16] * e[20] * e[17] + e[16] * e[11] * e[26] + e[16] * e[10] * e[25] + e[25] * e[11] * e[17] + e[25] * e[9] * e[15] - 1.*e[10] * e[23] * e[14] - 1.*e[10] * e[24] * e[15] - 1.*e[10] * e[26] * e[17] + e[10] * e[20] * e[11] + e[10] * e[18] * e[9] - 1.*e[10] * e[21] * e[12] + .5000000000*e[19] * ep2[11] + .5000000000*e[19] * ep2[9] + 1.500000000*e[19] * ep2[10] + .5000000000*e[19] * ep2[13] + .5000000000*e[19] * ep2[16] - .5000000000*e[19] * ep2[12] - .5000000000*e[19] * ep2[15] - .5000000000*e[19] * ep2[17] - .5000000000*e[19] * ep2[14];
			A[164] = e[10] * e[18] * e[6] + e[10] * e[0] * e[24] + e[10] * e[19] * e[7] + e[10] * e[1] * e[25] + e[10] * e[20] * e[8] + e[10] * e[2] * e[26] + e[19] * e[9] * e[6] + e[19] * e[0] * e[15] + e[19] * e[1] * e[16] + e[19] * e[11] * e[8] + e[19] * e[2] * e[17] + e[4] * e[21] * e[15] + e[4] * e[12] * e[24] + e[4] * e[23] * e[17] + e[4] * e[14] * e[26] + e[4] * e[22] * e[16] + e[4] * e[13] * e[25] + e[7] * e[24] * e[15] + e[7] * e[26] * e[17] + 3.*e[7] * e[25] * e[16] + e[1] * e[18] * e[15] + e[1] * e[9] * e[24] + e[1] * e[20] * e[17] + e[1] * e[11] * e[26] - 1.*e[16] * e[21] * e[3] + e[16] * e[26] * e[8] - 1.*e[16] * e[20] * e[2] - 1.*e[16] * e[18] * e[0] - 1.*e[16] * e[23] * e[5] + e[16] * e[24] * e[6] + e[13] * e[21] * e[6] + e[13] * e[3] * e[24] + e[13] * e[22] * e[7] + e[13] * e[23] * e[8] + e[13] * e[5] * e[26] - 1.*e[25] * e[11] * e[2] + e[25] * e[15] * e[6] - 1.*e[25] * e[9] * e[0] - 1.*e[25] * e[14] * e[5] - 1.*e[25] * e[12] * e[3] + e[25] * e[17] * e[8] + e[22] * e[12] * e[6] + e[22] * e[3] * e[15] + e[22] * e[14] * e[8] + e[22] * e[5] * e[17] - 1.*e[7] * e[23] * e[14] - 1.*e[7] * e[20] * e[11] - 1.*e[7] * e[18] * e[9] - 1.*e[7] * e[21] * e[12];
			A[55] = e[13] * e[9] * e[3] + e[13] * e[0] * e[12] + e[13] * e[10] * e[4] + e[13] * e[11] * e[5] + e[13] * e[2] * e[14] + e[16] * e[9] * e[6] + e[16] * e[0] * e[15] + e[16] * e[10] * e[7] + e[16] * e[11] * e[8] + e[16] * e[2] * e[17] + e[7] * e[11] * e[17] + e[7] * e[9] * e[15] + e[4] * e[11] * e[14] + e[4] * e[9] * e[12] + e[10] * e[9] * e[0] + e[10] * e[11] * e[2] - 1.*e[10] * e[15] * e[6] - 1.*e[10] * e[14] * e[5] - 1.*e[10] * e[12] * e[3] - 1.*e[10] * e[17] * e[8] + .5000000000*e[1] * ep2[11] + .5000000000*e[1] * ep2[9] + 1.500000000*e[1] * ep2[10] - .5000000000*e[1] * ep2[12] - .5000000000*e[1] * ep2[15] - .5000000000*e[1] * ep2[17] - .5000000000*e[1] * ep2[14] + .5000000000*e[1] * ep2[13] + .5000000000*e[1] * ep2[16];
			A[165] = e[1] * e[27] * e[6] + e[1] * e[0] * e[33] + e[1] * e[28] * e[7] + e[1] * e[29] * e[8] + e[1] * e[2] * e[35] - 1.*e[7] * e[27] * e[0] - 1.*e[7] * e[32] * e[5] + e[7] * e[33] * e[6] - 1.*e[7] * e[30] * e[3] + e[7] * e[35] * e[8] - 1.*e[7] * e[29] * e[2] + e[7] * e[31] * e[4] + e[28] * e[0] * e[6] + e[28] * e[2] * e[8] + e[4] * e[30] * e[6] + e[4] * e[3] * e[33] + e[4] * e[32] * e[8] + e[4] * e[5] * e[35] + e[31] * e[3] * e[6] + e[31] * e[5] * e[8] + .5000000000*ep2[1] * e[34] + 1.500000000*e[34] * ep2[7] + .5000000000*e[34] * ep2[4] - .5000000000*e[34] * ep2[0] + .5000000000*e[34] * ep2[6] - .5000000000*e[34] * ep2[5] - .5000000000*e[34] * ep2[3] - .5000000000*e[34] * ep2[2] + .5000000000*e[34] * ep2[8];
			A[52] = e[4] * e[18] * e[3] + e[4] * e[0] * e[21] + e[4] * e[1] * e[22] + e[4] * e[20] * e[5] + e[4] * e[2] * e[23] + e[22] * e[0] * e[3] + e[22] * e[2] * e[5] + e[7] * e[18] * e[6] + e[7] * e[0] * e[24] + e[7] * e[1] * e[25] + e[7] * e[20] * e[8] + e[7] * e[2] * e[26] + e[25] * e[0] * e[6] + e[25] * e[2] * e[8] + e[1] * e[18] * e[0] + e[1] * e[20] * e[2] - 1.*e[1] * e[21] * e[3] - 1.*e[1] * e[26] * e[8] - 1.*e[1] * e[23] * e[5] - 1.*e[1] * e[24] * e[6] + .5000000000*e[19] * ep2[4] + .5000000000*e[19] * ep2[0] - .5000000000*e[19] * ep2[6] - .5000000000*e[19] * ep2[5] + 1.500000000*e[19] * ep2[1] + .5000000000*e[19] * ep2[7] - .5000000000*e[19] * ep2[3] + .5000000000*e[19] * ep2[2] - .5000000000*e[19] * ep2[8];
			A[166] = -.5000000000*e[7] * ep2[0] + e[4] * e[5] * e[8] + .5000000000*ep2[4] * e[7] - .5000000000*e[7] * ep2[2] + .5000000000*e[7] * ep2[8] - .5000000000*e[7] * ep2[5] + .5000000000*e[7] * ep2[6] + e[1] * e[0] * e[6] + .5000000000*ep3[7] + e[4] * e[3] * e[6] + e[1] * e[2] * e[8] - .5000000000*e[7] * ep2[3] + .5000000000*ep2[1] * e[7];
			A[53] = -1.*e[1] * e[32] * e[23] - 1.*e[19] * e[32] * e[5] - 1.*e[19] * e[33] * e[6] - 1.*e[19] * e[30] * e[3] - 1.*e[19] * e[35] * e[8] - 1.*e[28] * e[21] * e[3] - 1.*e[28] * e[26] * e[8] - 1.*e[28] * e[23] * e[5] - 1.*e[28] * e[24] * e[6] + e[7] * e[27] * e[24] + e[7] * e[18] * e[33] + e[7] * e[29] * e[26] + e[7] * e[20] * e[35] + e[22] * e[27] * e[3] + e[22] * e[0] * e[30] + e[22] * e[29] * e[5] + e[22] * e[2] * e[32] + e[31] * e[18] * e[3] + e[31] * e[0] * e[21] + e[31] * e[20] * e[5] + e[31] * e[2] * e[23] + e[25] * e[27] * e[6] + e[25] * e[0] * e[33] + e[25] * e[28] * e[7] + e[25] * e[1] * e[34] + e[25] * e[29] * e[8] + e[25] * e[2] * e[35] + e[34] * e[18] * e[6] + e[34] * e[0] * e[24] + e[34] * e[19] * e[7] + e[34] * e[20] * e[8] + e[34] * e[2] * e[26] + e[1] * e[27] * e[18] + 3.*e[1] * e[28] * e[19] + e[1] * e[29] * e[20] + e[19] * e[27] * e[0] + e[19] * e[29] * e[2] + e[28] * e[18] * e[0] + e[28] * e[20] * e[2] + e[4] * e[27] * e[21] + e[4] * e[18] * e[30] + e[4] * e[28] * e[22] + e[4] * e[19] * e[31] + e[4] * e[29] * e[23] + e[4] * e[20] * e[32] - 1.*e[1] * e[33] * e[24] - 1.*e[1] * e[30] * e[21] - 1.*e[1] * e[35] * e[26] + e[1] * e[31] * e[22];
			A[167] = e[10] * e[27] * e[15] + e[10] * e[9] * e[33] + e[10] * e[29] * e[17] + e[10] * e[11] * e[35] + e[10] * e[28] * e[16] + e[28] * e[11] * e[17] + e[28] * e[9] * e[15] + e[13] * e[30] * e[15] + e[13] * e[12] * e[33] + e[13] * e[32] * e[17] + e[13] * e[14] * e[35] + e[13] * e[31] * e[16] + e[31] * e[14] * e[17] + e[31] * e[12] * e[15] + e[16] * e[33] * e[15] + e[16] * e[35] * e[17] - 1.*e[16] * e[27] * e[9] - 1.*e[16] * e[30] * e[12] - 1.*e[16] * e[32] * e[14] - 1.*e[16] * e[29] * e[11] + .5000000000*ep2[10] * e[34] + 1.500000000*e[34] * ep2[16] - .5000000000*e[34] * ep2[9] - .5000000000*e[34] * ep2[11] - .5000000000*e[34] * ep2[12] + .5000000000*e[34] * ep2[15] + .5000000000*e[34] * ep2[17] - .5000000000*e[34] * ep2[14] + .5000000000*e[34] * ep2[13];
			A[50] = .5000000000*e[19] * ep2[18] + .5000000000*e[19] * ep2[25] + .5000000000*e[19] * ep2[22] + e[25] * e[20] * e[26] - .5000000000*e[19] * ep2[21] + .5000000000*e[19] * ep2[20] - .5000000000*e[19] * ep2[26] - .5000000000*e[19] * ep2[23] - .5000000000*e[19] * ep2[24] + .5000000000*ep3[19] + e[22] * e[20] * e[23] + e[25] * e[18] * e[24] + e[22] * e[18] * e[21];
			A[160] = .5000000000*e[34] * ep2[33] + .5000000000*e[34] * ep2[35] - .5000000000*e[34] * ep2[27] - .5000000000*e[34] * ep2[32] - .5000000000*e[34] * ep2[29] - .5000000000*e[34] * ep2[30] + .5000000000*ep2[28] * e[34] + e[31] * e[30] * e[33] + e[31] * e[32] * e[35] + e[28] * e[27] * e[33] + .5000000000*ep3[34] + e[28] * e[29] * e[35] + .5000000000*ep2[31] * e[34];
			A[51] = e[4] * e[28] * e[13] + e[4] * e[10] * e[31] + e[7] * e[27] * e[15] + e[7] * e[9] * e[33] + e[7] * e[29] * e[17] + e[7] * e[11] * e[35] + e[7] * e[28] * e[16] + e[7] * e[10] * e[34] + e[1] * e[27] * e[9] + e[1] * e[29] * e[11] + 3.*e[1] * e[28] * e[10] + e[10] * e[27] * e[0] - 1.*e[10] * e[32] * e[5] - 1.*e[10] * e[33] * e[6] - 1.*e[10] * e[30] * e[3] - 1.*e[10] * e[35] * e[8] + e[10] * e[29] * e[2] + e[13] * e[27] * e[3] + e[13] * e[0] * e[30] + e[13] * e[1] * e[31] + e[13] * e[29] * e[5] + e[13] * e[2] * e[32] + e[28] * e[11] * e[2] - 1.*e[28] * e[15] * e[6] + e[28] * e[9] * e[0] - 1.*e[28] * e[14] * e[5] - 1.*e[28] * e[12] * e[3] - 1.*e[28] * e[17] * e[8] + e[31] * e[9] * e[3] + e[31] * e[0] * e[12] + e[31] * e[11] * e[5] + e[31] * e[2] * e[14] + e[16] * e[27] * e[6] + e[16] * e[0] * e[33] + e[16] * e[1] * e[34] + e[16] * e[29] * e[8] + e[16] * e[2] * e[35] - 1.*e[1] * e[30] * e[12] - 1.*e[1] * e[32] * e[14] - 1.*e[1] * e[33] * e[15] - 1.*e[1] * e[35] * e[17] + e[34] * e[9] * e[6] + e[34] * e[0] * e[15] + e[34] * e[11] * e[8] + e[34] * e[2] * e[17] + e[4] * e[27] * e[12] + e[4] * e[9] * e[30] + e[4] * e[29] * e[14] + e[4] * e[11] * e[32];
			A[161] = e[4] * e[30] * e[33] + e[4] * e[32] * e[35] + e[4] * e[31] * e[34] + e[31] * e[30] * e[6] + e[31] * e[3] * e[33] + e[31] * e[32] * e[8] + e[31] * e[5] * e[35] + e[28] * e[27] * e[6] + e[28] * e[0] * e[33] + e[28] * e[29] * e[8] + e[28] * e[2] * e[35] + e[34] * e[33] * e[6] + e[34] * e[35] * e[8] - 1.*e[34] * e[27] * e[0] - 1.*e[34] * e[32] * e[5] - 1.*e[34] * e[30] * e[3] - 1.*e[34] * e[29] * e[2] + e[1] * e[27] * e[33] + e[1] * e[29] * e[35] + e[1] * e[28] * e[34] + .5000000000*ep2[31] * e[7] - .5000000000*e[7] * ep2[27] - .5000000000*e[7] * ep2[32] + .5000000000*e[7] * ep2[28] - .5000000000*e[7] * ep2[29] + .5000000000*e[7] * ep2[33] - .5000000000*e[7] * ep2[30] + 1.500000000*e[7] * ep2[34] + .5000000000*e[7] * ep2[35];
			A[48] = -.5000000000*e[10] * ep2[14] - .5000000000*e[10] * ep2[17] - .5000000000*e[10] * ep2[15] + e[13] * e[11] * e[14] + e[16] * e[11] * e[17] + .5000000000*e[10] * ep2[13] + e[13] * e[9] * e[12] - .5000000000*e[10] * ep2[12] + .5000000000*ep3[10] + e[16] * e[9] * e[15] + .5000000000*e[10] * ep2[16] + .5000000000*e[10] * ep2[11] + .5000000000*e[10] * ep2[9];
			A[162] = e[22] * e[32] * e[35] + e[22] * e[31] * e[34] + e[31] * e[30] * e[24] + e[31] * e[21] * e[33] + e[31] * e[32] * e[26] + e[31] * e[23] * e[35] + e[34] * e[33] * e[24] + e[34] * e[35] * e[26] - 1.*e[34] * e[27] * e[18] - 1.*e[34] * e[30] * e[21] - 1.*e[34] * e[29] * e[20] - 1.*e[34] * e[32] * e[23] + e[19] * e[27] * e[33] + e[19] * e[29] * e[35] + e[19] * e[28] * e[34] + e[28] * e[27] * e[24] + e[28] * e[18] * e[33] + e[28] * e[29] * e[26] + e[28] * e[20] * e[35] + e[22] * e[30] * e[33] + .5000000000*ep2[28] * e[25] + .5000000000*ep2[31] * e[25] + .5000000000*e[25] * ep2[33] + .5000000000*e[25] * ep2[35] + 1.500000000*e[25] * ep2[34] - .5000000000*e[25] * ep2[27] - .5000000000*e[25] * ep2[32] - .5000000000*e[25] * ep2[29] - .5000000000*e[25] * ep2[30];
			A[49] = -1.*e[19] * e[35] * e[26] - 1.*e[19] * e[32] * e[23] + e[19] * e[27] * e[18] + e[19] * e[29] * e[20] + e[22] * e[27] * e[21] + e[22] * e[18] * e[30] + e[22] * e[19] * e[31] + e[22] * e[29] * e[23] + e[22] * e[20] * e[32] + e[31] * e[18] * e[21] + e[31] * e[20] * e[23] + e[25] * e[27] * e[24] + e[25] * e[18] * e[33] + e[25] * e[19] * e[34] + e[25] * e[29] * e[26] + e[25] * e[20] * e[35] + e[34] * e[18] * e[24] + e[34] * e[20] * e[26] - 1.*e[19] * e[33] * e[24] - 1.*e[19] * e[30] * e[21] + 1.500000000*e[28] * ep2[19] + .5000000000*e[28] * ep2[18] + .5000000000*e[28] * ep2[20] + .5000000000*e[28] * ep2[22] + .5000000000*e[28] * ep2[25] - .5000000000*e[28] * ep2[26] - .5000000000*e[28] * ep2[23] - .5000000000*e[28] * ep2[24] - .5000000000*e[28] * ep2[21];
			A[163] = e[10] * e[27] * e[33] + e[10] * e[29] * e[35] + e[10] * e[28] * e[34] + e[34] * e[33] * e[15] + e[34] * e[35] * e[17] + e[28] * e[27] * e[15] + e[28] * e[9] * e[33] + e[28] * e[29] * e[17] + e[28] * e[11] * e[35] - 1.*e[34] * e[27] * e[9] - 1.*e[34] * e[30] * e[12] + e[34] * e[31] * e[13] - 1.*e[34] * e[32] * e[14] - 1.*e[34] * e[29] * e[11] + e[31] * e[30] * e[15] + e[31] * e[12] * e[33] + e[31] * e[32] * e[17] + e[31] * e[14] * e[35] + e[13] * e[30] * e[33] + e[13] * e[32] * e[35] - .5000000000*e[16] * ep2[27] - .5000000000*e[16] * ep2[32] + .5000000000*e[16] * ep2[28] - .5000000000*e[16] * ep2[29] + .5000000000*e[16] * ep2[31] + .5000000000*e[16] * ep2[33] - .5000000000*e[16] * ep2[30] + 1.500000000*e[16] * ep2[34] + .5000000000*e[16] * ep2[35];
			A[63] = e[29] * e[32] * e[14] - 1.*e[29] * e[33] * e[15] - 1.*e[29] * e[34] * e[16] + e[32] * e[27] * e[12] + e[32] * e[9] * e[30] + e[32] * e[28] * e[13] + e[32] * e[10] * e[31] + e[14] * e[27] * e[30] + e[14] * e[28] * e[31] + e[17] * e[27] * e[33] + e[17] * e[28] * e[34] + e[35] * e[27] * e[15] + e[35] * e[9] * e[33] + e[35] * e[29] * e[17] + e[35] * e[28] * e[16] + e[35] * e[10] * e[34] + e[29] * e[27] * e[9] + e[29] * e[28] * e[10] - 1.*e[29] * e[30] * e[12] - 1.*e[29] * e[31] * e[13] + .5000000000*e[11] * ep2[27] + 1.500000000*e[11] * ep2[29] + .5000000000*e[11] * ep2[28] + .5000000000*e[11] * ep2[32] - .5000000000*e[11] * ep2[31] - .5000000000*e[11] * ep2[33] - .5000000000*e[11] * ep2[30] - .5000000000*e[11] * ep2[34] + .5000000000*e[11] * ep2[35];
			A[173] = e[1] * e[20] * e[35] + e[19] * e[27] * e[6] + e[19] * e[0] * e[33] + e[19] * e[28] * e[7] + e[19] * e[29] * e[8] + e[19] * e[2] * e[35] + e[28] * e[18] * e[6] + e[28] * e[0] * e[24] + e[28] * e[20] * e[8] + e[28] * e[2] * e[26] + e[4] * e[30] * e[24] + e[4] * e[21] * e[33] + e[4] * e[31] * e[25] + e[4] * e[22] * e[34] + e[4] * e[32] * e[26] + e[4] * e[23] * e[35] - 1.*e[7] * e[27] * e[18] + e[7] * e[33] * e[24] - 1.*e[7] * e[30] * e[21] - 1.*e[7] * e[29] * e[20] + e[7] * e[35] * e[26] + e[7] * e[31] * e[22] - 1.*e[7] * e[32] * e[23] - 1.*e[25] * e[27] * e[0] - 1.*e[25] * e[32] * e[5] - 1.*e[25] * e[30] * e[3] - 1.*e[25] * e[29] * e[2] - 1.*e[34] * e[21] * e[3] - 1.*e[34] * e[20] * e[2] - 1.*e[34] * e[18] * e[0] - 1.*e[34] * e[23] * e[5] + e[22] * e[30] * e[6] + e[22] * e[3] * e[33] + e[22] * e[32] * e[8] + e[22] * e[5] * e[35] + e[31] * e[21] * e[6] + e[31] * e[3] * e[24] + e[31] * e[23] * e[8] + e[31] * e[5] * e[26] + e[34] * e[26] * e[8] + e[1] * e[27] * e[24] + e[1] * e[18] * e[33] + e[1] * e[28] * e[25] + e[1] * e[19] * e[34] + e[1] * e[29] * e[26] + e[34] * e[24] * e[6] + e[25] * e[33] * e[6] + 3.*e[25] * e[34] * e[7] + e[25] * e[35] * e[8];
			A[62] = .5000000000*e[20] * ep2[27] + 1.500000000*e[20] * ep2[29] + .5000000000*e[20] * ep2[28] + .5000000000*e[20] * ep2[32] + .5000000000*e[20] * ep2[35] - .5000000000*e[20] * ep2[31] - .5000000000*e[20] * ep2[33] - .5000000000*e[20] * ep2[30] - .5000000000*e[20] * ep2[34] + e[29] * e[27] * e[18] + e[29] * e[28] * e[19] + e[23] * e[27] * e[30] + e[23] * e[29] * e[32] + e[23] * e[28] * e[31] + e[32] * e[27] * e[21] + e[32] * e[18] * e[30] + e[32] * e[28] * e[22] + e[32] * e[19] * e[31] + e[26] * e[27] * e[33] + e[26] * e[29] * e[35] + e[26] * e[28] * e[34] + e[35] * e[27] * e[24] + e[35] * e[18] * e[33] + e[35] * e[28] * e[25] + e[35] * e[19] * e[34] - 1.*e[29] * e[33] * e[24] - 1.*e[29] * e[30] * e[21] - 1.*e[29] * e[31] * e[22] - 1.*e[29] * e[34] * e[25];
			A[172] = e[19] * e[1] * e[7] + e[19] * e[0] * e[6] + e[19] * e[2] * e[8] + e[4] * e[21] * e[6] + e[4] * e[3] * e[24] + e[4] * e[22] * e[7] + e[4] * e[23] * e[8] + e[4] * e[5] * e[26] + e[22] * e[3] * e[6] + e[22] * e[5] * e[8] + e[7] * e[24] * e[6] + e[7] * e[26] * e[8] + e[1] * e[18] * e[6] + e[1] * e[0] * e[24] + e[1] * e[20] * e[8] + e[1] * e[2] * e[26] - 1.*e[7] * e[21] * e[3] - 1.*e[7] * e[20] * e[2] - 1.*e[7] * e[18] * e[0] - 1.*e[7] * e[23] * e[5] + .5000000000*e[25] * ep2[4] - .5000000000*e[25] * ep2[0] + .5000000000*e[25] * ep2[6] - .5000000000*e[25] * ep2[5] + .5000000000*e[25] * ep2[1] + 1.500000000*e[25] * ep2[7] - .5000000000*e[25] * ep2[3] - .5000000000*e[25] * ep2[2] + .5000000000*e[25] * ep2[8];
			A[61] = e[5] * e[27] * e[30] + e[5] * e[29] * e[32] + e[5] * e[28] * e[31] + e[32] * e[27] * e[3] + e[32] * e[0] * e[30] + e[32] * e[28] * e[4] + e[32] * e[1] * e[31] + e[8] * e[27] * e[33] + e[8] * e[29] * e[35] + e[8] * e[28] * e[34] + e[29] * e[27] * e[0] + e[29] * e[28] * e[1] + e[35] * e[27] * e[6] + e[35] * e[0] * e[33] + e[35] * e[28] * e[7] + e[35] * e[1] * e[34] - 1.*e[29] * e[34] * e[7] - 1.*e[29] * e[33] * e[6] - 1.*e[29] * e[30] * e[3] - 1.*e[29] * e[31] * e[4] + .5000000000*e[2] * ep2[27] + 1.500000000*e[2] * ep2[29] + .5000000000*e[2] * ep2[28] + .5000000000*e[2] * ep2[32] - .5000000000*e[2] * ep2[31] - .5000000000*e[2] * ep2[33] - .5000000000*e[2] * ep2[30] - .5000000000*e[2] * ep2[34] + .5000000000*e[2] * ep2[35];
			A[175] = e[13] * e[12] * e[6] + e[13] * e[3] * e[15] + e[13] * e[4] * e[16] + e[13] * e[14] * e[8] + e[13] * e[5] * e[17] + e[16] * e[15] * e[6] + e[16] * e[17] * e[8] + e[1] * e[11] * e[17] + e[1] * e[9] * e[15] + e[1] * e[10] * e[16] + e[4] * e[14] * e[17] + e[4] * e[12] * e[15] + e[10] * e[9] * e[6] + e[10] * e[0] * e[15] + e[10] * e[11] * e[8] + e[10] * e[2] * e[17] - 1.*e[16] * e[11] * e[2] - 1.*e[16] * e[9] * e[0] - 1.*e[16] * e[14] * e[5] - 1.*e[16] * e[12] * e[3] + .5000000000*ep2[13] * e[7] + 1.500000000*ep2[16] * e[7] + .5000000000*e[7] * ep2[17] + .5000000000*e[7] * ep2[15] - .5000000000*e[7] * ep2[9] - .5000000000*e[7] * ep2[11] - .5000000000*e[7] * ep2[12] + .5000000000*e[7] * ep2[10] - .5000000000*e[7] * ep2[14];
			A[60] = .5000000000*e[29] * ep2[32] + .5000000000*e[29] * ep2[35] - .5000000000*e[29] * ep2[31] - .5000000000*e[29] * ep2[33] - .5000000000*e[29] * ep2[30] - .5000000000*e[29] * ep2[34] + e[32] * e[27] * e[30] + .5000000000*ep3[29] + .5000000000*e[29] * ep2[28] + e[35] * e[28] * e[34] + .5000000000*e[29] * ep2[27] + e[35] * e[27] * e[33] + e[32] * e[28] * e[31];
			A[174] = -1.*e[16] * e[21] * e[12] + e[10] * e[18] * e[15] + e[10] * e[9] * e[24] + e[10] * e[20] * e[17] + e[10] * e[11] * e[26] + e[19] * e[11] * e[17] + e[19] * e[9] * e[15] + e[19] * e[10] * e[16] + e[13] * e[21] * e[15] + e[13] * e[12] * e[24] + e[13] * e[23] * e[17] + e[13] * e[14] * e[26] + e[13] * e[22] * e[16] + e[22] * e[14] * e[17] + e[22] * e[12] * e[15] + e[16] * e[24] * e[15] + e[16] * e[26] * e[17] - 1.*e[16] * e[23] * e[14] - 1.*e[16] * e[20] * e[11] - 1.*e[16] * e[18] * e[9] + .5000000000*ep2[13] * e[25] + 1.500000000*e[25] * ep2[16] + .5000000000*e[25] * ep2[17] + .5000000000*e[25] * ep2[15] + .5000000000*ep2[10] * e[25] - .5000000000*e[25] * ep2[9] - .5000000000*e[25] * ep2[11] - .5000000000*e[25] * ep2[12] - .5000000000*e[25] * ep2[14];
			A[59] = e[19] * e[20] * e[2] + e[22] * e[18] * e[3] + e[22] * e[0] * e[21] + e[22] * e[19] * e[4] + e[22] * e[20] * e[5] + e[22] * e[2] * e[23] - 1.*e[19] * e[21] * e[3] - 1.*e[19] * e[26] * e[8] + e[19] * e[25] * e[7] - 1.*e[19] * e[23] * e[5] - 1.*e[19] * e[24] * e[6] + e[4] * e[18] * e[21] + e[4] * e[20] * e[23] + e[25] * e[18] * e[6] + e[25] * e[0] * e[24] + e[25] * e[20] * e[8] + e[25] * e[2] * e[26] + e[7] * e[18] * e[24] + e[7] * e[20] * e[26] + e[19] * e[18] * e[0] + 1.500000000*ep2[19] * e[1] + .5000000000*e[1] * ep2[22] + .5000000000*e[1] * ep2[18] + .5000000000*e[1] * ep2[20] + .5000000000*e[1] * ep2[25] - .5000000000*e[1] * ep2[26] - .5000000000*e[1] * ep2[23] - .5000000000*e[1] * ep2[24] - .5000000000*e[1] * ep2[21];
			A[169] = e[19] * e[27] * e[24] + e[19] * e[18] * e[33] + e[19] * e[28] * e[25] + e[19] * e[29] * e[26] + e[19] * e[20] * e[35] + e[28] * e[18] * e[24] + e[28] * e[20] * e[26] + e[22] * e[30] * e[24] + e[22] * e[21] * e[33] + e[22] * e[31] * e[25] + e[22] * e[32] * e[26] + e[22] * e[23] * e[35] + e[31] * e[21] * e[24] + e[31] * e[23] * e[26] + e[25] * e[33] * e[24] + e[25] * e[35] * e[26] - 1.*e[25] * e[27] * e[18] - 1.*e[25] * e[30] * e[21] - 1.*e[25] * e[29] * e[20] - 1.*e[25] * e[32] * e[23] - .5000000000*e[34] * ep2[18] - .5000000000*e[34] * ep2[23] - .5000000000*e[34] * ep2[20] - .5000000000*e[34] * ep2[21] + .5000000000*ep2[19] * e[34] + .5000000000*ep2[22] * e[34] + 1.500000000*e[34] * ep2[25] + .5000000000*e[34] * ep2[24] + .5000000000*e[34] * ep2[26];
			A[58] = e[16] * e[0] * e[6] + e[16] * e[2] * e[8] + e[1] * e[11] * e[2] - 1.*e[1] * e[15] * e[6] + e[1] * e[9] * e[0] - 1.*e[1] * e[14] * e[5] - 1.*e[1] * e[12] * e[3] - 1.*e[1] * e[17] * e[8] + e[4] * e[9] * e[3] + e[4] * e[0] * e[12] + e[4] * e[1] * e[13] + e[4] * e[11] * e[5] + e[4] * e[2] * e[14] + e[13] * e[0] * e[3] + e[13] * e[2] * e[5] + e[7] * e[9] * e[6] + e[7] * e[0] * e[15] + e[7] * e[1] * e[16] + e[7] * e[11] * e[8] + e[7] * e[2] * e[17] - .5000000000*e[10] * ep2[6] - .5000000000*e[10] * ep2[5] - .5000000000*e[10] * ep2[3] - .5000000000*e[10] * ep2[8] + 1.500000000*e[10] * ep2[1] + .5000000000*e[10] * ep2[0] + .5000000000*e[10] * ep2[2] + .5000000000*e[10] * ep2[4] + .5000000000*e[10] * ep2[7];
			A[168] = e[13] * e[14] * e[17] + e[13] * e[12] * e[15] + e[10] * e[9] * e[15] + .5000000000*e[16] * ep2[15] - .5000000000*e[16] * ep2[11] - .5000000000*e[16] * ep2[12] - .5000000000*e[16] * ep2[14] + e[10] * e[11] * e[17] + .5000000000*ep2[10] * e[16] + .5000000000*ep3[16] - .5000000000*e[16] * ep2[9] + .5000000000*e[16] * ep2[17] + .5000000000*ep2[13] * e[16];
			A[57] = e[10] * e[29] * e[20] + e[22] * e[27] * e[12] + e[22] * e[9] * e[30] + e[22] * e[29] * e[14] + e[22] * e[11] * e[32] + e[22] * e[10] * e[31] + e[31] * e[18] * e[12] + e[31] * e[9] * e[21] + e[31] * e[20] * e[14] + e[31] * e[11] * e[23] - 1.*e[10] * e[33] * e[24] - 1.*e[10] * e[30] * e[21] - 1.*e[10] * e[35] * e[26] - 1.*e[10] * e[32] * e[23] + e[10] * e[34] * e[25] + e[19] * e[27] * e[9] + e[19] * e[29] * e[11] + e[28] * e[18] * e[9] + e[28] * e[20] * e[11] + e[16] * e[27] * e[24] + e[16] * e[18] * e[33] + e[16] * e[28] * e[25] + e[16] * e[19] * e[34] + e[16] * e[29] * e[26] + e[16] * e[20] * e[35] - 1.*e[19] * e[30] * e[12] - 1.*e[19] * e[32] * e[14] - 1.*e[19] * e[33] * e[15] - 1.*e[19] * e[35] * e[17] - 1.*e[28] * e[23] * e[14] - 1.*e[28] * e[24] * e[15] - 1.*e[28] * e[26] * e[17] - 1.*e[28] * e[21] * e[12] + e[25] * e[27] * e[15] + e[25] * e[9] * e[33] + e[25] * e[29] * e[17] + e[25] * e[11] * e[35] + e[34] * e[18] * e[15] + e[34] * e[9] * e[24] + e[34] * e[20] * e[17] + e[34] * e[11] * e[26] + e[13] * e[27] * e[21] + e[13] * e[18] * e[30] + e[13] * e[28] * e[22] + e[13] * e[19] * e[31] + e[13] * e[29] * e[23] + e[13] * e[20] * e[32] + e[10] * e[27] * e[18] + 3.*e[10] * e[28] * e[19];
			A[171] = e[4] * e[30] * e[15] + e[4] * e[12] * e[33] + e[4] * e[32] * e[17] + e[4] * e[14] * e[35] + e[4] * e[31] * e[16] + e[4] * e[13] * e[34] + e[7] * e[33] * e[15] + e[7] * e[35] * e[17] + 3.*e[7] * e[34] * e[16] + e[1] * e[27] * e[15] + e[1] * e[9] * e[33] + e[1] * e[29] * e[17] + e[1] * e[11] * e[35] + e[1] * e[28] * e[16] + e[1] * e[10] * e[34] - 1.*e[16] * e[27] * e[0] - 1.*e[16] * e[32] * e[5] + e[16] * e[33] * e[6] - 1.*e[16] * e[30] * e[3] + e[16] * e[35] * e[8] - 1.*e[16] * e[29] * e[2] + e[13] * e[30] * e[6] + e[13] * e[3] * e[33] + e[13] * e[31] * e[7] + e[13] * e[32] * e[8] + e[13] * e[5] * e[35] - 1.*e[34] * e[11] * e[2] + e[34] * e[15] * e[6] - 1.*e[34] * e[9] * e[0] - 1.*e[34] * e[14] * e[5] - 1.*e[34] * e[12] * e[3] + e[34] * e[17] * e[8] + e[31] * e[12] * e[6] + e[31] * e[3] * e[15] + e[31] * e[14] * e[8] + e[31] * e[5] * e[17] - 1.*e[7] * e[27] * e[9] - 1.*e[7] * e[30] * e[12] + e[7] * e[28] * e[10] - 1.*e[7] * e[32] * e[14] + e[10] * e[27] * e[6] + e[10] * e[0] * e[33] + e[10] * e[29] * e[8] + e[10] * e[2] * e[35] + e[28] * e[9] * e[6] + e[28] * e[0] * e[15] + e[28] * e[11] * e[8] + e[28] * e[2] * e[17] - 1.*e[7] * e[29] * e[11];
			A[56] = e[22] * e[18] * e[12] + e[22] * e[9] * e[21] + e[22] * e[20] * e[14] + e[22] * e[11] * e[23] + e[22] * e[19] * e[13] + e[25] * e[18] * e[15] + e[25] * e[9] * e[24] + e[25] * e[20] * e[17] + e[25] * e[11] * e[26] + e[25] * e[19] * e[16] + e[16] * e[18] * e[24] + e[16] * e[20] * e[26] + e[13] * e[18] * e[21] + e[13] * e[20] * e[23] + e[19] * e[18] * e[9] + e[19] * e[20] * e[11] - 1.*e[19] * e[23] * e[14] - 1.*e[19] * e[24] * e[15] - 1.*e[19] * e[26] * e[17] - 1.*e[19] * e[21] * e[12] + .5000000000*e[10] * ep2[22] + .5000000000*e[10] * ep2[25] + 1.500000000*e[10] * ep2[19] + .5000000000*e[10] * ep2[18] + .5000000000*e[10] * ep2[20] - .5000000000*e[10] * ep2[26] - .5000000000*e[10] * ep2[23] - .5000000000*e[10] * ep2[24] - .5000000000*e[10] * ep2[21];
			A[170] = e[19] * e[20] * e[26] - .5000000000*e[25] * ep2[20] + e[22] * e[21] * e[24] + e[19] * e[18] * e[24] + .5000000000*ep2[22] * e[25] - .5000000000*e[25] * ep2[21] - .5000000000*e[25] * ep2[23] + .5000000000*ep2[19] * e[25] - .5000000000*e[25] * ep2[18] + .5000000000*e[25] * ep2[24] + .5000000000*e[25] * ep2[26] + .5000000000*ep3[25] + e[22] * e[23] * e[26];
			A[73] = -1.*e[20] * e[33] * e[6] - 1.*e[20] * e[30] * e[3] - 1.*e[20] * e[31] * e[4] - 1.*e[29] * e[21] * e[3] - 1.*e[29] * e[22] * e[4] - 1.*e[29] * e[25] * e[7] - 1.*e[29] * e[24] * e[6] + e[8] * e[27] * e[24] + e[8] * e[18] * e[33] + e[8] * e[28] * e[25] + e[8] * e[19] * e[34] + e[23] * e[27] * e[3] + e[23] * e[0] * e[30] + e[23] * e[28] * e[4] + e[23] * e[1] * e[31] + e[32] * e[18] * e[3] + e[32] * e[0] * e[21] + e[32] * e[19] * e[4] + e[32] * e[1] * e[22] + e[26] * e[27] * e[6] + e[26] * e[0] * e[33] + e[26] * e[28] * e[7] + e[26] * e[1] * e[34] + e[26] * e[29] * e[8] + e[26] * e[2] * e[35] + e[35] * e[18] * e[6] + e[35] * e[0] * e[24] + e[35] * e[19] * e[7] + e[35] * e[1] * e[25] + e[35] * e[20] * e[8] + e[2] * e[27] * e[18] + e[2] * e[28] * e[19] + 3.*e[2] * e[29] * e[20] + e[20] * e[27] * e[0] + e[20] * e[28] * e[1] + e[29] * e[18] * e[0] + e[29] * e[19] * e[1] + e[5] * e[27] * e[21] + e[5] * e[18] * e[30] + e[5] * e[28] * e[22] + e[5] * e[19] * e[31] + e[5] * e[29] * e[23] + e[5] * e[20] * e[32] - 1.*e[2] * e[33] * e[24] - 1.*e[2] * e[30] * e[21] - 1.*e[2] * e[31] * e[22] + e[2] * e[32] * e[23] - 1.*e[2] * e[34] * e[25] - 1.*e[20] * e[34] * e[7];
			A[72] = e[5] * e[18] * e[3] + e[5] * e[0] * e[21] + e[5] * e[19] * e[4] + e[5] * e[1] * e[22] + e[5] * e[2] * e[23] + e[23] * e[1] * e[4] + e[23] * e[0] * e[3] + e[8] * e[18] * e[6] + e[8] * e[0] * e[24] + e[8] * e[19] * e[7] + e[8] * e[1] * e[25] + e[8] * e[2] * e[26] + e[26] * e[1] * e[7] + e[26] * e[0] * e[6] + e[2] * e[18] * e[0] + e[2] * e[19] * e[1] - 1.*e[2] * e[21] * e[3] - 1.*e[2] * e[22] * e[4] - 1.*e[2] * e[25] * e[7] - 1.*e[2] * e[24] * e[6] - .5000000000*e[20] * ep2[4] + .5000000000*e[20] * ep2[0] - .5000000000*e[20] * ep2[6] + .5000000000*e[20] * ep2[5] + .5000000000*e[20] * ep2[1] - .5000000000*e[20] * ep2[7] - .5000000000*e[20] * ep2[3] + 1.500000000*e[20] * ep2[2] + .5000000000*e[20] * ep2[8];
			A[75] = e[14] * e[9] * e[3] + e[14] * e[0] * e[12] + e[14] * e[10] * e[4] + e[14] * e[1] * e[13] + e[14] * e[11] * e[5] + e[17] * e[9] * e[6] + e[17] * e[0] * e[15] + e[17] * e[10] * e[7] + e[17] * e[1] * e[16] + e[17] * e[11] * e[8] + e[8] * e[9] * e[15] + e[8] * e[10] * e[16] + e[5] * e[9] * e[12] + e[5] * e[10] * e[13] + e[11] * e[9] * e[0] + e[11] * e[10] * e[1] - 1.*e[11] * e[13] * e[4] - 1.*e[11] * e[16] * e[7] - 1.*e[11] * e[15] * e[6] - 1.*e[11] * e[12] * e[3] + .5000000000*e[2] * ep2[14] + .5000000000*e[2] * ep2[17] + 1.500000000*e[2] * ep2[11] + .5000000000*e[2] * ep2[9] + .5000000000*e[2] * ep2[10] - .5000000000*e[2] * ep2[16] - .5000000000*e[2] * ep2[12] - .5000000000*e[2] * ep2[15] - .5000000000*e[2] * ep2[13];
			A[74] = e[14] * e[18] * e[12] + e[14] * e[9] * e[21] + e[14] * e[11] * e[23] + e[14] * e[19] * e[13] + e[14] * e[10] * e[22] + e[23] * e[9] * e[12] + e[23] * e[10] * e[13] + e[17] * e[18] * e[15] + e[17] * e[9] * e[24] + e[17] * e[11] * e[26] + e[17] * e[19] * e[16] + e[17] * e[10] * e[25] + e[26] * e[9] * e[15] + e[26] * e[10] * e[16] - 1.*e[11] * e[24] * e[15] - 1.*e[11] * e[25] * e[16] + e[11] * e[18] * e[9] - 1.*e[11] * e[21] * e[12] + e[11] * e[19] * e[10] - 1.*e[11] * e[22] * e[13] + 1.500000000*e[20] * ep2[11] + .5000000000*e[20] * ep2[9] + .5000000000*e[20] * ep2[10] + .5000000000*e[20] * ep2[14] + .5000000000*e[20] * ep2[17] - .5000000000*e[20] * ep2[16] - .5000000000*e[20] * ep2[12] - .5000000000*e[20] * ep2[15] - .5000000000*e[20] * ep2[13];
			A[77] = e[23] * e[10] * e[31] + e[32] * e[18] * e[12] + e[32] * e[9] * e[21] + e[32] * e[19] * e[13] + e[32] * e[10] * e[22] - 1.*e[11] * e[33] * e[24] - 1.*e[11] * e[30] * e[21] + e[11] * e[35] * e[26] - 1.*e[11] * e[31] * e[22] - 1.*e[11] * e[34] * e[25] + e[20] * e[27] * e[9] + e[20] * e[28] * e[10] + e[29] * e[18] * e[9] + e[29] * e[19] * e[10] + e[17] * e[27] * e[24] + e[17] * e[18] * e[33] + e[17] * e[28] * e[25] + e[17] * e[19] * e[34] + e[17] * e[29] * e[26] + e[17] * e[20] * e[35] - 1.*e[20] * e[30] * e[12] - 1.*e[20] * e[31] * e[13] - 1.*e[20] * e[33] * e[15] - 1.*e[20] * e[34] * e[16] - 1.*e[29] * e[24] * e[15] - 1.*e[29] * e[25] * e[16] - 1.*e[29] * e[21] * e[12] - 1.*e[29] * e[22] * e[13] + e[26] * e[27] * e[15] + e[26] * e[9] * e[33] + e[26] * e[28] * e[16] + e[26] * e[10] * e[34] + e[35] * e[18] * e[15] + e[35] * e[9] * e[24] + e[35] * e[19] * e[16] + e[35] * e[10] * e[25] + e[14] * e[27] * e[21] + e[14] * e[18] * e[30] + e[14] * e[28] * e[22] + e[14] * e[19] * e[31] + e[14] * e[29] * e[23] + e[14] * e[20] * e[32] + e[11] * e[27] * e[18] + e[11] * e[28] * e[19] + 3.*e[11] * e[29] * e[20] + e[23] * e[27] * e[12] + e[23] * e[9] * e[30] + e[23] * e[11] * e[32] + e[23] * e[28] * e[13];
			A[76] = e[23] * e[18] * e[12] + e[23] * e[9] * e[21] + e[23] * e[20] * e[14] + e[23] * e[19] * e[13] + e[23] * e[10] * e[22] + e[26] * e[18] * e[15] + e[26] * e[9] * e[24] + e[26] * e[20] * e[17] + e[26] * e[19] * e[16] + e[26] * e[10] * e[25] + e[17] * e[19] * e[25] + e[17] * e[18] * e[24] + e[14] * e[19] * e[22] + e[14] * e[18] * e[21] + e[20] * e[18] * e[9] + e[20] * e[19] * e[10] - 1.*e[20] * e[24] * e[15] - 1.*e[20] * e[25] * e[16] - 1.*e[20] * e[21] * e[12] - 1.*e[20] * e[22] * e[13] + .5000000000*e[11] * ep2[23] + .5000000000*e[11] * ep2[26] + .5000000000*e[11] * ep2[19] + .5000000000*e[11] * ep2[18] + 1.500000000*e[11] * ep2[20] - .5000000000*e[11] * ep2[22] - .5000000000*e[11] * ep2[24] - .5000000000*e[11] * ep2[21] - .5000000000*e[11] * ep2[25];
			A[79] = -1.*e[20] * e[21] * e[3] + e[20] * e[26] * e[8] - 1.*e[20] * e[22] * e[4] - 1.*e[20] * e[25] * e[7] - 1.*e[20] * e[24] * e[6] + e[5] * e[19] * e[22] + e[5] * e[18] * e[21] + e[26] * e[18] * e[6] + e[26] * e[0] * e[24] + e[26] * e[19] * e[7] + e[26] * e[1] * e[25] + e[8] * e[19] * e[25] + e[8] * e[18] * e[24] + e[20] * e[18] * e[0] + e[20] * e[19] * e[1] + e[23] * e[18] * e[3] + e[23] * e[0] * e[21] + e[23] * e[19] * e[4] + e[23] * e[1] * e[22] + e[23] * e[20] * e[5] + 1.500000000*ep2[20] * e[2] + .5000000000*e[2] * ep2[23] + .5000000000*e[2] * ep2[19] + .5000000000*e[2] * ep2[18] + .5000000000*e[2] * ep2[26] - .5000000000*e[2] * ep2[22] - .5000000000*e[2] * ep2[24] - .5000000000*e[2] * ep2[21] - .5000000000*e[2] * ep2[25];
			A[78] = -1.*e[2] * e[15] * e[6] + e[2] * e[9] * e[0] - 1.*e[2] * e[12] * e[3] + e[5] * e[9] * e[3] + e[5] * e[0] * e[12] + e[5] * e[10] * e[4] + e[5] * e[1] * e[13] + e[5] * e[2] * e[14] + e[14] * e[1] * e[4] + e[14] * e[0] * e[3] + e[8] * e[9] * e[6] + e[8] * e[0] * e[15] + e[8] * e[10] * e[7] + e[8] * e[1] * e[16] + e[8] * e[2] * e[17] + e[17] * e[1] * e[7] + e[17] * e[0] * e[6] + e[2] * e[10] * e[1] - 1.*e[2] * e[13] * e[4] - 1.*e[2] * e[16] * e[7] + .5000000000*e[11] * ep2[1] + .5000000000*e[11] * ep2[0] + 1.500000000*e[11] * ep2[2] + .5000000000*e[11] * ep2[5] + .5000000000*e[11] * ep2[8] - .5000000000*e[11] * ep2[4] - .5000000000*e[11] * ep2[6] - .5000000000*e[11] * ep2[7] - .5000000000*e[11] * ep2[3];
			A[64] = e[5] * e[19] * e[13] + e[5] * e[10] * e[22] + e[8] * e[18] * e[15] + e[8] * e[9] * e[24] + e[8] * e[20] * e[17] + e[8] * e[11] * e[26] + e[8] * e[19] * e[16] + e[8] * e[10] * e[25] + e[2] * e[18] * e[9] + e[2] * e[19] * e[10] - 1.*e[11] * e[21] * e[3] - 1.*e[11] * e[22] * e[4] - 1.*e[11] * e[25] * e[7] - 1.*e[11] * e[24] * e[6] + e[14] * e[18] * e[3] + e[14] * e[0] * e[21] + e[14] * e[19] * e[4] + e[14] * e[1] * e[22] + e[14] * e[2] * e[23] - 1.*e[20] * e[13] * e[4] - 1.*e[20] * e[16] * e[7] - 1.*e[20] * e[15] * e[6] - 1.*e[20] * e[12] * e[3] + e[23] * e[9] * e[3] + e[23] * e[0] * e[12] + e[23] * e[10] * e[4] + e[23] * e[1] * e[13] + e[17] * e[18] * e[6] + e[17] * e[0] * e[24] + e[17] * e[19] * e[7] + e[17] * e[1] * e[25] + e[17] * e[2] * e[26] - 1.*e[2] * e[24] * e[15] - 1.*e[2] * e[25] * e[16] - 1.*e[2] * e[21] * e[12] - 1.*e[2] * e[22] * e[13] + e[26] * e[9] * e[6] + e[26] * e[0] * e[15] + e[26] * e[10] * e[7] + e[26] * e[1] * e[16] + e[11] * e[18] * e[0] + e[11] * e[19] * e[1] + 3.*e[11] * e[20] * e[2] + e[20] * e[9] * e[0] + e[20] * e[10] * e[1] + e[5] * e[18] * e[12] + e[5] * e[9] * e[21] + e[5] * e[20] * e[14] + e[5] * e[11] * e[23];
			A[65] = e[32] * e[1] * e[4] + e[32] * e[0] * e[3] + e[8] * e[27] * e[6] + e[8] * e[0] * e[33] + e[8] * e[28] * e[7] + e[8] * e[1] * e[34] + e[35] * e[1] * e[7] + e[35] * e[0] * e[6] + e[2] * e[27] * e[0] + e[2] * e[28] * e[1] - 1.*e[2] * e[34] * e[7] + e[2] * e[32] * e[5] - 1.*e[2] * e[33] * e[6] - 1.*e[2] * e[30] * e[3] + e[2] * e[35] * e[8] - 1.*e[2] * e[31] * e[4] + e[5] * e[27] * e[3] + e[5] * e[0] * e[30] + e[5] * e[28] * e[4] + e[5] * e[1] * e[31] + 1.500000000*e[29] * ep2[2] - .5000000000*e[29] * ep2[4] + .5000000000*e[29] * ep2[0] - .5000000000*e[29] * ep2[6] + .5000000000*e[29] * ep2[5] + .5000000000*e[29] * ep2[1] - .5000000000*e[29] * ep2[7] - .5000000000*e[29] * ep2[3] + .5000000000*e[29] * ep2[8];
			A[66] = e[5] * e[0] * e[3] + e[8] * e[1] * e[7] + e[8] * e[0] * e[6] + e[5] * e[1] * e[4] - .5000000000*e[2] * ep2[4] + .5000000000*ep3[2] + .5000000000*e[2] * ep2[1] - .5000000000*e[2] * ep2[3] + .5000000000*e[2] * ep2[0] + .5000000000*e[2] * ep2[8] + .5000000000*e[2] * ep2[5] - .5000000000*e[2] * ep2[6] - .5000000000*e[2] * ep2[7];
			A[67] = e[35] * e[9] * e[15] + e[35] * e[10] * e[16] - 1.*e[11] * e[30] * e[12] - 1.*e[11] * e[31] * e[13] - 1.*e[11] * e[33] * e[15] - 1.*e[11] * e[34] * e[16] + e[11] * e[27] * e[9] + e[11] * e[28] * e[10] + e[14] * e[27] * e[12] + e[14] * e[9] * e[30] + e[14] * e[11] * e[32] + e[14] * e[28] * e[13] + e[14] * e[10] * e[31] + e[32] * e[9] * e[12] + e[32] * e[10] * e[13] + e[17] * e[27] * e[15] + e[17] * e[9] * e[33] + e[17] * e[11] * e[35] + e[17] * e[28] * e[16] + e[17] * e[10] * e[34] + 1.500000000*e[29] * ep2[11] - .5000000000*e[29] * ep2[16] + .5000000000*e[29] * ep2[9] - .5000000000*e[29] * ep2[12] - .5000000000*e[29] * ep2[15] + .5000000000*e[29] * ep2[17] + .5000000000*e[29] * ep2[10] + .5000000000*e[29] * ep2[14] - .5000000000*e[29] * ep2[13];
			A[68] = e[14] * e[9] * e[12] + e[17] * e[10] * e[16] + e[17] * e[9] * e[15] + .5000000000*ep3[11] + e[14] * e[10] * e[13] + .5000000000*e[11] * ep2[10] - .5000000000*e[11] * ep2[15] + .5000000000*e[11] * ep2[14] - .5000000000*e[11] * ep2[13] - .5000000000*e[11] * ep2[12] + .5000000000*e[11] * ep2[9] - .5000000000*e[11] * ep2[16] + .5000000000*e[11] * ep2[17];
			A[69] = e[20] * e[27] * e[18] + e[20] * e[28] * e[19] + e[23] * e[27] * e[21] + e[23] * e[18] * e[30] + e[23] * e[28] * e[22] + e[23] * e[19] * e[31] + e[23] * e[20] * e[32] + e[32] * e[19] * e[22] + e[32] * e[18] * e[21] + e[26] * e[27] * e[24] + e[26] * e[18] * e[33] + e[26] * e[28] * e[25] + e[26] * e[19] * e[34] + e[26] * e[20] * e[35] + e[35] * e[19] * e[25] + e[35] * e[18] * e[24] - 1.*e[20] * e[33] * e[24] - 1.*e[20] * e[30] * e[21] - 1.*e[20] * e[31] * e[22] - 1.*e[20] * e[34] * e[25] + .5000000000*e[29] * ep2[23] + .5000000000*e[29] * ep2[26] - .5000000000*e[29] * ep2[22] - .5000000000*e[29] * ep2[24] - .5000000000*e[29] * ep2[21] - .5000000000*e[29] * ep2[25] + 1.500000000*e[29] * ep2[20] + .5000000000*e[29] * ep2[19] + .5000000000*e[29] * ep2[18];
			A[70] = .5000000000*e[20] * ep2[26] + .5000000000*e[20] * ep2[18] + .5000000000*ep3[20] + .5000000000*e[20] * ep2[19] + e[26] * e[18] * e[24] + .5000000000*e[20] * ep2[23] - .5000000000*e[20] * ep2[25] + e[23] * e[19] * e[22] - .5000000000*e[20] * ep2[24] - .5000000000*e[20] * ep2[21] - .5000000000*e[20] * ep2[22] + e[23] * e[18] * e[21] + e[26] * e[19] * e[25];
			A[71] = e[8] * e[28] * e[16] + e[8] * e[10] * e[34] + e[2] * e[27] * e[9] + 3.*e[2] * e[29] * e[11] + e[2] * e[28] * e[10] + e[11] * e[27] * e[0] - 1.*e[11] * e[34] * e[7] - 1.*e[11] * e[33] * e[6] - 1.*e[11] * e[30] * e[3] + e[11] * e[28] * e[1] - 1.*e[11] * e[31] * e[4] + e[14] * e[27] * e[3] + e[14] * e[0] * e[30] + e[14] * e[28] * e[4] + e[14] * e[1] * e[31] + e[14] * e[2] * e[32] + e[29] * e[10] * e[1] - 1.*e[29] * e[13] * e[4] - 1.*e[29] * e[16] * e[7] - 1.*e[29] * e[15] * e[6] + e[29] * e[9] * e[0] - 1.*e[29] * e[12] * e[3] + e[32] * e[9] * e[3] + e[32] * e[0] * e[12] + e[32] * e[10] * e[4] + e[32] * e[1] * e[13] + e[17] * e[27] * e[6] + e[17] * e[0] * e[33] + e[17] * e[28] * e[7] + e[17] * e[1] * e[34] + e[17] * e[2] * e[35] - 1.*e[2] * e[30] * e[12] - 1.*e[2] * e[31] * e[13] - 1.*e[2] * e[33] * e[15] - 1.*e[2] * e[34] * e[16] + e[35] * e[9] * e[6] + e[35] * e[0] * e[15] + e[35] * e[10] * e[7] + e[35] * e[1] * e[16] + e[5] * e[27] * e[12] + e[5] * e[9] * e[30] + e[5] * e[29] * e[14] + e[5] * e[11] * e[32] + e[5] * e[28] * e[13] + e[5] * e[10] * e[31] + e[8] * e[27] * e[15] + e[8] * e[9] * e[33] + e[8] * e[29] * e[17] + e[8] * e[11] * e[35];
			A[91] = -1.*e[12] * e[34] * e[7] + e[12] * e[32] * e[5] - 1.*e[12] * e[35] * e[8] - 1.*e[12] * e[29] * e[2] - 1.*e[12] * e[28] * e[1] + e[12] * e[31] * e[4] - 1.*e[30] * e[11] * e[2] - 1.*e[30] * e[10] * e[1] + e[30] * e[13] * e[4] - 1.*e[30] * e[16] * e[7] + e[30] * e[14] * e[5] - 1.*e[30] * e[17] * e[8] + e[15] * e[3] * e[33] + e[15] * e[31] * e[7] + e[15] * e[4] * e[34] + e[15] * e[32] * e[8] + e[15] * e[5] * e[35] + e[3] * e[27] * e[9] - 1.*e[3] * e[28] * e[10] - 1.*e[3] * e[34] * e[16] - 1.*e[3] * e[35] * e[17] - 1.*e[3] * e[29] * e[11] + e[33] * e[13] * e[7] + e[33] * e[4] * e[16] + e[33] * e[14] * e[8] + e[33] * e[5] * e[17] + e[9] * e[28] * e[4] + e[9] * e[1] * e[31] + e[9] * e[29] * e[5] + e[9] * e[2] * e[32] + e[27] * e[10] * e[4] + e[27] * e[1] * e[13] + e[27] * e[11] * e[5] + e[27] * e[2] * e[14] + 3.*e[3] * e[30] * e[12] + e[3] * e[32] * e[14] + e[3] * e[31] * e[13] + e[6] * e[30] * e[15] + e[6] * e[12] * e[33] + e[6] * e[32] * e[17] + e[6] * e[14] * e[35] + e[6] * e[31] * e[16] + e[6] * e[13] * e[34] + e[0] * e[27] * e[12] + e[0] * e[9] * e[30] + e[0] * e[29] * e[14] + e[0] * e[11] * e[32] + e[0] * e[28] * e[13] + e[0] * e[10] * e[31];
			A[90] = .5000000000*e[21] * ep2[24] - .5000000000*e[21] * ep2[25] + .5000000000*e[21] * ep2[23] - .5000000000*e[21] * ep2[26] + .5000000000*ep2[18] * e[21] + .5000000000*e[21] * ep2[22] - .5000000000*e[21] * ep2[20] + e[24] * e[22] * e[25] + e[24] * e[23] * e[26] - .5000000000*e[21] * ep2[19] + e[18] * e[19] * e[22] + e[18] * e[20] * e[23] + .5000000000*ep3[21];
			A[89] = -.5000000000*e[30] * ep2[26] - .5000000000*e[30] * ep2[19] - .5000000000*e[30] * ep2[20] - .5000000000*e[30] * ep2[25] + .5000000000*ep2[18] * e[30] + 1.500000000*e[30] * ep2[21] + .5000000000*e[30] * ep2[22] + .5000000000*e[30] * ep2[23] + .5000000000*e[30] * ep2[24] + e[18] * e[27] * e[21] + e[18] * e[28] * e[22] + e[18] * e[19] * e[31] + e[18] * e[29] * e[23] + e[18] * e[20] * e[32] + e[27] * e[19] * e[22] + e[27] * e[20] * e[23] + e[21] * e[31] * e[22] + e[21] * e[32] * e[23] + e[24] * e[21] * e[33] + e[24] * e[31] * e[25] + e[24] * e[22] * e[34] + e[24] * e[32] * e[26] + e[24] * e[23] * e[35] + e[33] * e[22] * e[25] + e[33] * e[23] * e[26] - 1.*e[21] * e[29] * e[20] - 1.*e[21] * e[35] * e[26] - 1.*e[21] * e[28] * e[19] - 1.*e[21] * e[34] * e[25];
			A[88] = .5000000000*e[12] * ep2[15] - .5000000000*e[12] * ep2[17] + e[15] * e[13] * e[16] - .5000000000*e[12] * ep2[10] + e[15] * e[14] * e[17] - .5000000000*e[12] * ep2[16] - .5000000000*e[12] * ep2[11] + e[9] * e[10] * e[13] + .5000000000*e[12] * ep2[13] + .5000000000*ep2[9] * e[12] + .5000000000*ep3[12] + e[9] * e[11] * e[14] + .5000000000*e[12] * ep2[14];
			A[95] = e[12] * e[13] * e[4] + e[12] * e[14] * e[5] + e[15] * e[12] * e[6] + e[15] * e[13] * e[7] + e[15] * e[4] * e[16] + e[15] * e[14] * e[8] + e[15] * e[5] * e[17] + e[6] * e[14] * e[17] + e[6] * e[13] * e[16] + e[0] * e[11] * e[14] + e[0] * e[9] * e[12] + e[0] * e[10] * e[13] + e[9] * e[10] * e[4] + e[9] * e[1] * e[13] + e[9] * e[11] * e[5] + e[9] * e[2] * e[14] - 1.*e[12] * e[11] * e[2] - 1.*e[12] * e[10] * e[1] - 1.*e[12] * e[16] * e[7] - 1.*e[12] * e[17] * e[8] + 1.500000000*ep2[12] * e[3] + .5000000000*e[3] * ep2[15] - .5000000000*e[3] * ep2[16] + .5000000000*e[3] * ep2[9] - .5000000000*e[3] * ep2[11] - .5000000000*e[3] * ep2[17] - .5000000000*e[3] * ep2[10] + .5000000000*e[3] * ep2[14] + .5000000000*e[3] * ep2[13];
			A[94] = e[18] * e[11] * e[14] + e[18] * e[9] * e[12] + e[18] * e[10] * e[13] + e[12] * e[23] * e[14] + e[12] * e[22] * e[13] + e[15] * e[12] * e[24] + e[15] * e[23] * e[17] + e[15] * e[14] * e[26] + e[15] * e[22] * e[16] + e[15] * e[13] * e[25] + e[24] * e[14] * e[17] + e[24] * e[13] * e[16] - 1.*e[12] * e[25] * e[16] - 1.*e[12] * e[26] * e[17] - 1.*e[12] * e[20] * e[11] - 1.*e[12] * e[19] * e[10] + e[9] * e[20] * e[14] + e[9] * e[11] * e[23] + e[9] * e[19] * e[13] + e[9] * e[10] * e[22] + .5000000000*ep2[9] * e[21] - .5000000000*e[21] * ep2[16] - .5000000000*e[21] * ep2[11] - .5000000000*e[21] * ep2[17] - .5000000000*e[21] * ep2[10] + 1.500000000*e[21] * ep2[12] + .5000000000*e[21] * ep2[14] + .5000000000*e[21] * ep2[13] + .5000000000*e[21] * ep2[15];
			A[93] = -1.*e[21] * e[35] * e[8] - 1.*e[21] * e[29] * e[2] - 1.*e[21] * e[28] * e[1] + e[21] * e[31] * e[4] - 1.*e[30] * e[26] * e[8] - 1.*e[30] * e[20] * e[2] - 1.*e[30] * e[19] * e[1] + e[30] * e[22] * e[4] - 1.*e[30] * e[25] * e[7] + e[30] * e[23] * e[5] + e[6] * e[31] * e[25] + e[6] * e[22] * e[34] + e[6] * e[32] * e[26] + e[6] * e[23] * e[35] + e[24] * e[30] * e[6] + e[24] * e[3] * e[33] + e[24] * e[31] * e[7] + e[24] * e[4] * e[34] + e[24] * e[32] * e[8] + e[24] * e[5] * e[35] + e[33] * e[21] * e[6] + e[33] * e[22] * e[7] + e[33] * e[4] * e[25] + e[33] * e[23] * e[8] + e[33] * e[5] * e[26] + e[0] * e[27] * e[21] + e[0] * e[18] * e[30] + e[0] * e[28] * e[22] + e[0] * e[19] * e[31] + e[0] * e[29] * e[23] + e[0] * e[20] * e[32] + e[18] * e[27] * e[3] + e[18] * e[28] * e[4] + e[18] * e[1] * e[31] + e[18] * e[29] * e[5] + e[18] * e[2] * e[32] + e[27] * e[19] * e[4] + e[27] * e[1] * e[22] + e[27] * e[20] * e[5] + e[27] * e[2] * e[23] + 3.*e[3] * e[30] * e[21] + e[3] * e[31] * e[22] + e[3] * e[32] * e[23] - 1.*e[3] * e[29] * e[20] - 1.*e[3] * e[35] * e[26] - 1.*e[3] * e[28] * e[19] - 1.*e[3] * e[34] * e[25] - 1.*e[21] * e[34] * e[7] + e[21] * e[32] * e[5];
			A[92] = e[18] * e[1] * e[4] + e[18] * e[0] * e[3] + e[18] * e[2] * e[5] + e[3] * e[22] * e[4] + e[3] * e[23] * e[5] + e[6] * e[3] * e[24] + e[6] * e[22] * e[7] + e[6] * e[4] * e[25] + e[6] * e[23] * e[8] + e[6] * e[5] * e[26] + e[24] * e[4] * e[7] + e[24] * e[5] * e[8] + e[0] * e[19] * e[4] + e[0] * e[1] * e[22] + e[0] * e[20] * e[5] + e[0] * e[2] * e[23] - 1.*e[3] * e[26] * e[8] - 1.*e[3] * e[20] * e[2] - 1.*e[3] * e[19] * e[1] - 1.*e[3] * e[25] * e[7] + .5000000000*e[21] * ep2[4] + .5000000000*e[21] * ep2[0] + .5000000000*e[21] * ep2[6] + .5000000000*e[21] * ep2[5] - .5000000000*e[21] * ep2[1] - .5000000000*e[21] * ep2[7] + 1.500000000*e[21] * ep2[3] - .5000000000*e[21] * ep2[2] - .5000000000*e[21] * ep2[8];
			A[82] = .5000000000*ep2[27] * e[21] + 1.500000000*e[21] * ep2[30] + .5000000000*e[21] * ep2[32] + .5000000000*e[21] * ep2[31] + .5000000000*e[21] * ep2[33] - .5000000000*e[21] * ep2[28] - .5000000000*e[21] * ep2[29] - .5000000000*e[21] * ep2[34] - .5000000000*e[21] * ep2[35] + e[18] * e[27] * e[30] + e[18] * e[29] * e[32] + e[18] * e[28] * e[31] + e[27] * e[28] * e[22] + e[27] * e[19] * e[31] + e[27] * e[29] * e[23] + e[27] * e[20] * e[32] + e[30] * e[31] * e[22] + e[30] * e[32] * e[23] + e[24] * e[30] * e[33] + e[24] * e[32] * e[35] + e[24] * e[31] * e[34] + e[33] * e[31] * e[25] + e[33] * e[22] * e[34] + e[33] * e[32] * e[26] + e[33] * e[23] * e[35] - 1.*e[30] * e[29] * e[20] - 1.*e[30] * e[35] * e[26] - 1.*e[30] * e[28] * e[19] - 1.*e[30] * e[34] * e[25];
			A[192] = -.5000000000*e[26] * ep2[4] - .5000000000*e[26] * ep2[0] + .5000000000*e[26] * ep2[6] + .5000000000*e[26] * ep2[5] - .5000000000*e[26] * ep2[1] + .5000000000*e[26] * ep2[7] - .5000000000*e[26] * ep2[3] + .5000000000*e[26] * ep2[2] + 1.500000000*e[26] * ep2[8] + e[20] * e[0] * e[6] + e[20] * e[2] * e[8] + e[5] * e[21] * e[6] + e[5] * e[3] * e[24] + e[5] * e[22] * e[7] + e[5] * e[4] * e[25] + e[5] * e[23] * e[8] + e[23] * e[4] * e[7] + e[23] * e[3] * e[6] + e[8] * e[24] * e[6] + e[8] * e[25] * e[7] + e[2] * e[18] * e[6] + e[2] * e[0] * e[24] + e[2] * e[19] * e[7] + e[2] * e[1] * e[25] - 1.*e[8] * e[21] * e[3] - 1.*e[8] * e[19] * e[1] - 1.*e[8] * e[22] * e[4] - 1.*e[8] * e[18] * e[0] + e[20] * e[1] * e[7];
			A[83] = e[9] * e[27] * e[30] + e[9] * e[29] * e[32] + e[9] * e[28] * e[31] + e[33] * e[30] * e[15] + e[33] * e[32] * e[17] + e[33] * e[14] * e[35] + e[33] * e[31] * e[16] + e[33] * e[13] * e[34] + e[27] * e[29] * e[14] + e[27] * e[11] * e[32] + e[27] * e[28] * e[13] + e[27] * e[10] * e[31] - 1.*e[30] * e[28] * e[10] + e[30] * e[31] * e[13] + e[30] * e[32] * e[14] - 1.*e[30] * e[34] * e[16] - 1.*e[30] * e[35] * e[17] - 1.*e[30] * e[29] * e[11] + e[15] * e[32] * e[35] + e[15] * e[31] * e[34] - .5000000000*e[12] * ep2[34] - .5000000000*e[12] * ep2[35] + .5000000000*e[12] * ep2[27] + .5000000000*e[12] * ep2[32] - .5000000000*e[12] * ep2[28] - .5000000000*e[12] * ep2[29] + .5000000000*e[12] * ep2[31] + .5000000000*e[12] * ep2[33] + 1.500000000*e[12] * ep2[30];
			A[193] = e[23] * e[30] * e[6] + e[23] * e[3] * e[33] + e[23] * e[31] * e[7] + e[23] * e[4] * e[34] + e[32] * e[21] * e[6] + e[32] * e[3] * e[24] + e[32] * e[22] * e[7] + e[32] * e[4] * e[25] + e[26] * e[33] * e[6] + e[26] * e[34] * e[7] + 3.*e[26] * e[35] * e[8] + e[35] * e[24] * e[6] + e[35] * e[25] * e[7] + e[2] * e[27] * e[24] + e[2] * e[18] * e[33] + e[2] * e[28] * e[25] + e[2] * e[19] * e[34] + e[2] * e[29] * e[26] + e[2] * e[20] * e[35] + e[20] * e[27] * e[6] + e[20] * e[0] * e[33] + e[20] * e[28] * e[7] + e[20] * e[1] * e[34] + e[20] * e[29] * e[8] + e[29] * e[18] * e[6] + e[29] * e[0] * e[24] + e[29] * e[19] * e[7] + e[29] * e[1] * e[25] + e[5] * e[30] * e[24] + e[5] * e[21] * e[33] + e[5] * e[31] * e[25] + e[5] * e[22] * e[34] + e[5] * e[32] * e[26] + e[5] * e[23] * e[35] - 1.*e[8] * e[27] * e[18] + e[8] * e[33] * e[24] - 1.*e[8] * e[30] * e[21] - 1.*e[8] * e[31] * e[22] + e[8] * e[32] * e[23] - 1.*e[8] * e[28] * e[19] + e[8] * e[34] * e[25] - 1.*e[26] * e[27] * e[0] - 1.*e[26] * e[30] * e[3] - 1.*e[26] * e[28] * e[1] - 1.*e[26] * e[31] * e[4] - 1.*e[35] * e[21] * e[3] - 1.*e[35] * e[19] * e[1] - 1.*e[35] * e[22] * e[4] - 1.*e[35] * e[18] * e[0];
			A[80] = e[27] * e[29] * e[32] + e[27] * e[28] * e[31] + e[33] * e[32] * e[35] + e[33] * e[31] * e[34] + .5000000000*ep3[30] - .5000000000*e[30] * ep2[28] - .5000000000*e[30] * ep2[29] - .5000000000*e[30] * ep2[34] + .5000000000*e[30] * ep2[33] + .5000000000*ep2[27] * e[30] + .5000000000*e[30] * ep2[32] + .5000000000*e[30] * ep2[31] - .5000000000*e[30] * ep2[35];
			A[194] = .5000000000*ep2[14] * e[26] + 1.500000000*e[26] * ep2[17] + .5000000000*e[26] * ep2[15] + .5000000000*e[26] * ep2[16] + .5000000000*ep2[11] * e[26] - .5000000000*e[26] * ep2[9] - .5000000000*e[26] * ep2[12] - .5000000000*e[26] * ep2[10] - .5000000000*e[26] * ep2[13] + e[20] * e[11] * e[17] + e[20] * e[9] * e[15] + e[20] * e[10] * e[16] + e[14] * e[21] * e[15] + e[14] * e[12] * e[24] + e[14] * e[23] * e[17] + e[14] * e[22] * e[16] + e[14] * e[13] * e[25] + e[23] * e[12] * e[15] + e[23] * e[13] * e[16] + e[17] * e[24] * e[15] + e[17] * e[25] * e[16] - 1.*e[17] * e[18] * e[9] - 1.*e[17] * e[21] * e[12] - 1.*e[17] * e[19] * e[10] - 1.*e[17] * e[22] * e[13] + e[11] * e[18] * e[15] + e[11] * e[9] * e[24] + e[11] * e[19] * e[16] + e[11] * e[10] * e[25];
			A[81] = e[0] * e[27] * e[30] + e[0] * e[29] * e[32] + e[0] * e[28] * e[31] + e[30] * e[31] * e[4] + e[30] * e[32] * e[5] + e[6] * e[30] * e[33] + e[6] * e[32] * e[35] + e[6] * e[31] * e[34] + e[27] * e[28] * e[4] + e[27] * e[1] * e[31] + e[27] * e[29] * e[5] + e[27] * e[2] * e[32] + e[33] * e[31] * e[7] + e[33] * e[4] * e[34] + e[33] * e[32] * e[8] + e[33] * e[5] * e[35] - 1.*e[30] * e[34] * e[7] - 1.*e[30] * e[35] * e[8] - 1.*e[30] * e[29] * e[2] - 1.*e[30] * e[28] * e[1] + 1.500000000*e[3] * ep2[30] + .5000000000*e[3] * ep2[32] + .5000000000*e[3] * ep2[31] + .5000000000*e[3] * ep2[27] - .5000000000*e[3] * ep2[28] - .5000000000*e[3] * ep2[29] + .5000000000*e[3] * ep2[33] - .5000000000*e[3] * ep2[34] - .5000000000*e[3] * ep2[35];
			A[195] = .5000000000*ep2[14] * e[8] + 1.500000000*ep2[17] * e[8] + .5000000000*e[8] * ep2[15] + .5000000000*e[8] * ep2[16] - .5000000000*e[8] * ep2[9] + .5000000000*e[8] * ep2[11] - .5000000000*e[8] * ep2[12] - .5000000000*e[8] * ep2[10] - .5000000000*e[8] * ep2[13] + e[14] * e[12] * e[6] + e[14] * e[3] * e[15] + e[14] * e[13] * e[7] + e[14] * e[4] * e[16] + e[14] * e[5] * e[17] + e[17] * e[15] * e[6] + e[17] * e[16] * e[7] + e[2] * e[11] * e[17] + e[2] * e[9] * e[15] + e[2] * e[10] * e[16] + e[5] * e[12] * e[15] + e[5] * e[13] * e[16] + e[11] * e[9] * e[6] + e[11] * e[0] * e[15] + e[11] * e[10] * e[7] + e[11] * e[1] * e[16] - 1.*e[17] * e[10] * e[1] - 1.*e[17] * e[13] * e[4] - 1.*e[17] * e[9] * e[0] - 1.*e[17] * e[12] * e[3];
			A[86] = -.5000000000*e[3] * ep2[1] - .5000000000*e[3] * ep2[7] + .5000000000*ep3[3] - .5000000000*e[3] * ep2[8] + e[0] * e[2] * e[5] + .5000000000*e[3] * ep2[6] + .5000000000*e[3] * ep2[4] - .5000000000*e[3] * ep2[2] + e[0] * e[1] * e[4] + e[6] * e[4] * e[7] + .5000000000*ep2[0] * e[3] + .5000000000*e[3] * ep2[5] + e[6] * e[5] * e[8];
			A[196] = .5000000000*ep2[23] * e[17] + 1.500000000*ep2[26] * e[17] + .5000000000*e[17] * ep2[25] + .5000000000*e[17] * ep2[24] - .5000000000*e[17] * ep2[18] - .5000000000*e[17] * ep2[19] + .5000000000*e[17] * ep2[20] - .5000000000*e[17] * ep2[22] - .5000000000*e[17] * ep2[21] + e[23] * e[21] * e[15] + e[23] * e[12] * e[24] + e[23] * e[14] * e[26] + e[23] * e[22] * e[16] + e[23] * e[13] * e[25] + e[26] * e[24] * e[15] + e[26] * e[25] * e[16] + e[11] * e[19] * e[25] + e[11] * e[18] * e[24] + e[11] * e[20] * e[26] + e[14] * e[22] * e[25] + e[14] * e[21] * e[24] + e[20] * e[18] * e[15] + e[20] * e[9] * e[24] + e[20] * e[19] * e[16] + e[20] * e[10] * e[25] - 1.*e[26] * e[18] * e[9] - 1.*e[26] * e[21] * e[12] - 1.*e[26] * e[19] * e[10] - 1.*e[26] * e[22] * e[13];
			A[87] = -1.*e[12] * e[34] * e[16] - 1.*e[12] * e[35] * e[17] - 1.*e[12] * e[29] * e[11] + e[9] * e[27] * e[12] + e[9] * e[29] * e[14] + e[9] * e[11] * e[32] + e[9] * e[28] * e[13] + e[9] * e[10] * e[31] + e[27] * e[11] * e[14] + e[27] * e[10] * e[13] + e[12] * e[32] * e[14] + e[12] * e[31] * e[13] + e[15] * e[12] * e[33] + e[15] * e[32] * e[17] + e[15] * e[14] * e[35] + e[15] * e[31] * e[16] + e[15] * e[13] * e[34] + e[33] * e[14] * e[17] + e[33] * e[13] * e[16] - 1.*e[12] * e[28] * e[10] + .5000000000*ep2[9] * e[30] - .5000000000*e[30] * ep2[16] - .5000000000*e[30] * ep2[11] + 1.500000000*e[30] * ep2[12] + .5000000000*e[30] * ep2[15] - .5000000000*e[30] * ep2[17] - .5000000000*e[30] * ep2[10] + .5000000000*e[30] * ep2[14] + .5000000000*e[30] * ep2[13];
			A[197] = e[32] * e[22] * e[16] + e[32] * e[13] * e[25] - 1.*e[17] * e[27] * e[18] + e[17] * e[33] * e[24] - 1.*e[17] * e[30] * e[21] + e[17] * e[29] * e[20] + 3.*e[17] * e[35] * e[26] - 1.*e[17] * e[31] * e[22] - 1.*e[17] * e[28] * e[19] + e[17] * e[34] * e[25] + e[20] * e[27] * e[15] + e[20] * e[9] * e[33] + e[20] * e[28] * e[16] + e[20] * e[10] * e[34] + e[29] * e[18] * e[15] + e[29] * e[9] * e[24] + e[29] * e[19] * e[16] + e[29] * e[10] * e[25] - 1.*e[26] * e[27] * e[9] - 1.*e[26] * e[30] * e[12] - 1.*e[26] * e[28] * e[10] - 1.*e[26] * e[31] * e[13] + e[26] * e[33] * e[15] + e[26] * e[34] * e[16] + e[35] * e[24] * e[15] + e[35] * e[25] * e[16] - 1.*e[35] * e[18] * e[9] - 1.*e[35] * e[21] * e[12] - 1.*e[35] * e[19] * e[10] - 1.*e[35] * e[22] * e[13] + e[14] * e[30] * e[24] + e[14] * e[21] * e[33] + e[14] * e[31] * e[25] + e[14] * e[22] * e[34] + e[14] * e[32] * e[26] + e[14] * e[23] * e[35] + e[11] * e[27] * e[24] + e[11] * e[18] * e[33] + e[11] * e[28] * e[25] + e[11] * e[19] * e[34] + e[11] * e[29] * e[26] + e[11] * e[20] * e[35] + e[23] * e[30] * e[15] + e[23] * e[12] * e[33] + e[23] * e[32] * e[17] + e[23] * e[31] * e[16] + e[23] * e[13] * e[34] + e[32] * e[21] * e[15] + e[32] * e[12] * e[24];
			A[84] = e[6] * e[23] * e[17] + e[6] * e[14] * e[26] + e[6] * e[22] * e[16] + e[6] * e[13] * e[25] + e[0] * e[20] * e[14] + e[0] * e[11] * e[23] + e[0] * e[19] * e[13] + e[0] * e[10] * e[22] - 1.*e[12] * e[26] * e[8] - 1.*e[12] * e[20] * e[2] - 1.*e[12] * e[19] * e[1] + e[12] * e[22] * e[4] - 1.*e[12] * e[25] * e[7] + e[12] * e[23] * e[5] - 1.*e[21] * e[11] * e[2] - 1.*e[21] * e[10] * e[1] + e[21] * e[13] * e[4] - 1.*e[21] * e[16] * e[7] + e[21] * e[14] * e[5] - 1.*e[21] * e[17] * e[8] + e[15] * e[3] * e[24] + e[15] * e[22] * e[7] + e[15] * e[4] * e[25] + e[15] * e[23] * e[8] + e[15] * e[5] * e[26] - 1.*e[3] * e[25] * e[16] - 1.*e[3] * e[26] * e[17] - 1.*e[3] * e[20] * e[11] - 1.*e[3] * e[19] * e[10] + e[24] * e[13] * e[7] + e[24] * e[4] * e[16] + e[24] * e[14] * e[8] + e[24] * e[5] * e[17] + e[9] * e[18] * e[3] + e[9] * e[0] * e[21] + e[9] * e[19] * e[4] + e[9] * e[1] * e[22] + e[9] * e[20] * e[5] + e[9] * e[2] * e[23] + e[18] * e[0] * e[12] + e[18] * e[10] * e[4] + e[18] * e[1] * e[13] + e[18] * e[11] * e[5] + e[18] * e[2] * e[14] + 3.*e[3] * e[21] * e[12] + e[3] * e[23] * e[14] + e[3] * e[22] * e[13] + e[6] * e[21] * e[15] + e[6] * e[12] * e[24];
			A[198] = .5000000000*ep2[5] * e[17] + 1.500000000*e[17] * ep2[8] + .5000000000*e[17] * ep2[7] + .5000000000*e[17] * ep2[6] + .5000000000*ep2[2] * e[17] - .5000000000*e[17] * ep2[4] - .5000000000*e[17] * ep2[0] - .5000000000*e[17] * ep2[1] - .5000000000*e[17] * ep2[3] + e[11] * e[1] * e[7] + e[11] * e[0] * e[6] + e[11] * e[2] * e[8] + e[5] * e[12] * e[6] + e[5] * e[3] * e[15] + e[5] * e[13] * e[7] + e[5] * e[4] * e[16] + e[5] * e[14] * e[8] + e[14] * e[4] * e[7] + e[14] * e[3] * e[6] + e[8] * e[15] * e[6] + e[8] * e[16] * e[7] - 1.*e[8] * e[10] * e[1] - 1.*e[8] * e[13] * e[4] - 1.*e[8] * e[9] * e[0] - 1.*e[8] * e[12] * e[3] + e[2] * e[9] * e[6] + e[2] * e[0] * e[15] + e[2] * e[10] * e[7] + e[2] * e[1] * e[16];
			A[85] = e[6] * e[4] * e[34] + e[6] * e[32] * e[8] + e[6] * e[5] * e[35] + e[33] * e[4] * e[7] + e[33] * e[5] * e[8] + e[0] * e[27] * e[3] + e[0] * e[28] * e[4] + e[0] * e[1] * e[31] + e[0] * e[29] * e[5] + e[0] * e[2] * e[32] - 1.*e[3] * e[34] * e[7] + e[3] * e[32] * e[5] + e[3] * e[33] * e[6] - 1.*e[3] * e[35] * e[8] - 1.*e[3] * e[29] * e[2] - 1.*e[3] * e[28] * e[1] + e[3] * e[31] * e[4] + e[27] * e[1] * e[4] + e[27] * e[2] * e[5] + e[6] * e[31] * e[7] + .5000000000*e[30] * ep2[4] + .5000000000*e[30] * ep2[6] + .5000000000*e[30] * ep2[5] - .5000000000*e[30] * ep2[1] - .5000000000*e[30] * ep2[7] - .5000000000*e[30] * ep2[2] - .5000000000*e[30] * ep2[8] + .5000000000*ep2[0] * e[30] + 1.500000000*e[30] * ep2[3];
			A[199] = .5000000000*ep2[23] * e[8] + 1.500000000*ep2[26] * e[8] - .5000000000*e[8] * ep2[18] - .5000000000*e[8] * ep2[19] - .5000000000*e[8] * ep2[22] + .5000000000*e[8] * ep2[24] - .5000000000*e[8] * ep2[21] + .5000000000*e[8] * ep2[25] + .5000000000*ep2[20] * e[8] + e[20] * e[18] * e[6] + e[20] * e[0] * e[24] + e[20] * e[19] * e[7] + e[20] * e[1] * e[25] + e[20] * e[2] * e[26] + e[23] * e[21] * e[6] + e[23] * e[3] * e[24] + e[23] * e[22] * e[7] + e[23] * e[4] * e[25] + e[23] * e[5] * e[26] - 1.*e[26] * e[21] * e[3] - 1.*e[26] * e[19] * e[1] - 1.*e[26] * e[22] * e[4] - 1.*e[26] * e[18] * e[0] + e[26] * e[25] * e[7] + e[26] * e[24] * e[6] + e[2] * e[19] * e[25] + e[2] * e[18] * e[24] + e[5] * e[22] * e[25] + e[5] * e[21] * e[24];
			A[109] = e[19] * e[27] * e[21] + e[19] * e[18] * e[30] + e[19] * e[28] * e[22] + e[19] * e[29] * e[23] + e[19] * e[20] * e[32] + e[28] * e[18] * e[21] + e[28] * e[20] * e[23] + e[22] * e[30] * e[21] + e[22] * e[32] * e[23] + e[25] * e[30] * e[24] + e[25] * e[21] * e[33] + e[25] * e[22] * e[34] + e[25] * e[32] * e[26] + e[25] * e[23] * e[35] + e[34] * e[21] * e[24] + e[34] * e[23] * e[26] - 1.*e[22] * e[27] * e[18] - 1.*e[22] * e[33] * e[24] - 1.*e[22] * e[29] * e[20] - 1.*e[22] * e[35] * e[26] + .5000000000*ep2[19] * e[31] + 1.500000000*e[31] * ep2[22] + .5000000000*e[31] * ep2[21] + .5000000000*e[31] * ep2[23] + .5000000000*e[31] * ep2[25] - .5000000000*e[31] * ep2[26] - .5000000000*e[31] * ep2[18] - .5000000000*e[31] * ep2[20] - .5000000000*e[31] * ep2[24];
			A[108] = -.5000000000*e[13] * ep2[15] + .5000000000*e[13] * ep2[16] + .5000000000*e[13] * ep2[12] + e[16] * e[12] * e[15] + .5000000000*ep3[13] + e[10] * e[11] * e[14] + .5000000000*e[13] * ep2[14] - .5000000000*e[13] * ep2[17] - .5000000000*e[13] * ep2[11] - .5000000000*e[13] * ep2[9] + .5000000000*ep2[10] * e[13] + e[10] * e[9] * e[12] + e[16] * e[14] * e[17];
			A[111] = -1.*e[13] * e[29] * e[2] - 1.*e[31] * e[11] * e[2] - 1.*e[31] * e[15] * e[6] - 1.*e[31] * e[9] * e[0] + e[31] * e[14] * e[5] + e[31] * e[12] * e[3] - 1.*e[31] * e[17] * e[8] + e[16] * e[30] * e[6] + e[16] * e[3] * e[33] + e[16] * e[4] * e[34] + e[16] * e[32] * e[8] + e[16] * e[5] * e[35] - 1.*e[4] * e[27] * e[9] + e[4] * e[28] * e[10] - 1.*e[4] * e[33] * e[15] - 1.*e[4] * e[35] * e[17] - 1.*e[4] * e[29] * e[11] + e[34] * e[12] * e[6] + e[34] * e[3] * e[15] + e[34] * e[14] * e[8] + e[34] * e[5] * e[17] + e[10] * e[27] * e[3] + e[10] * e[0] * e[30] + e[10] * e[29] * e[5] + e[10] * e[2] * e[32] + e[28] * e[9] * e[3] + e[28] * e[0] * e[12] + e[28] * e[11] * e[5] + e[28] * e[2] * e[14] + e[4] * e[30] * e[12] + e[4] * e[32] * e[14] + 3.*e[4] * e[31] * e[13] + e[7] * e[30] * e[15] + e[7] * e[12] * e[33] + e[7] * e[32] * e[17] + e[7] * e[14] * e[35] + e[7] * e[31] * e[16] + e[7] * e[13] * e[34] + e[1] * e[27] * e[12] + e[1] * e[9] * e[30] + e[1] * e[29] * e[14] + e[1] * e[11] * e[32] + e[1] * e[28] * e[13] + e[1] * e[10] * e[31] - 1.*e[13] * e[27] * e[0] + e[13] * e[32] * e[5] - 1.*e[13] * e[33] * e[6] + e[13] * e[30] * e[3] - 1.*e[13] * e[35] * e[8];
			A[110] = e[25] * e[23] * e[26] + e[19] * e[20] * e[23] + e[19] * e[18] * e[21] + e[25] * e[21] * e[24] + .5000000000*ep3[22] + .5000000000*e[22] * ep2[23] + .5000000000*ep2[19] * e[22] - .5000000000*e[22] * ep2[18] - .5000000000*e[22] * ep2[24] + .5000000000*e[22] * ep2[21] + .5000000000*e[22] * ep2[25] - .5000000000*e[22] * ep2[20] - .5000000000*e[22] * ep2[26];
			A[105] = e[34] * e[5] * e[8] + e[1] * e[27] * e[3] + e[1] * e[0] * e[30] + e[1] * e[28] * e[4] + e[1] * e[29] * e[5] + e[1] * e[2] * e[32] - 1.*e[4] * e[27] * e[0] + e[4] * e[34] * e[7] + e[4] * e[32] * e[5] - 1.*e[4] * e[33] * e[6] + e[4] * e[30] * e[3] - 1.*e[4] * e[35] * e[8] - 1.*e[4] * e[29] * e[2] + e[28] * e[0] * e[3] + e[28] * e[2] * e[5] + e[7] * e[30] * e[6] + e[7] * e[3] * e[33] + e[7] * e[32] * e[8] + e[7] * e[5] * e[35] + e[34] * e[3] * e[6] + .5000000000*ep2[1] * e[31] + 1.500000000*e[31] * ep2[4] - .5000000000*e[31] * ep2[0] - .5000000000*e[31] * ep2[6] + .5000000000*e[31] * ep2[5] + .5000000000*e[31] * ep2[7] + .5000000000*e[31] * ep2[3] - .5000000000*e[31] * ep2[2] - .5000000000*e[31] * ep2[8];
			A[104] = e[1] * e[20] * e[14] + e[1] * e[11] * e[23] + e[13] * e[21] * e[3] - 1.*e[13] * e[26] * e[8] - 1.*e[13] * e[20] * e[2] - 1.*e[13] * e[18] * e[0] + e[13] * e[23] * e[5] - 1.*e[13] * e[24] * e[6] - 1.*e[22] * e[11] * e[2] - 1.*e[22] * e[15] * e[6] - 1.*e[22] * e[9] * e[0] + e[22] * e[14] * e[5] + e[22] * e[12] * e[3] - 1.*e[22] * e[17] * e[8] + e[16] * e[21] * e[6] + e[16] * e[3] * e[24] + e[16] * e[4] * e[25] + e[16] * e[23] * e[8] + e[16] * e[5] * e[26] - 1.*e[4] * e[24] * e[15] - 1.*e[4] * e[26] * e[17] - 1.*e[4] * e[20] * e[11] - 1.*e[4] * e[18] * e[9] + e[25] * e[12] * e[6] + e[25] * e[3] * e[15] + e[25] * e[14] * e[8] + e[25] * e[5] * e[17] + e[10] * e[18] * e[3] + e[10] * e[0] * e[21] + e[10] * e[19] * e[4] + e[10] * e[1] * e[22] + e[10] * e[20] * e[5] + e[10] * e[2] * e[23] + e[19] * e[9] * e[3] + e[19] * e[0] * e[12] + e[19] * e[1] * e[13] + e[19] * e[11] * e[5] + e[19] * e[2] * e[14] + e[4] * e[21] * e[12] + e[4] * e[23] * e[14] + 3.*e[4] * e[22] * e[13] + e[7] * e[21] * e[15] + e[7] * e[12] * e[24] + e[7] * e[23] * e[17] + e[7] * e[14] * e[26] + e[7] * e[22] * e[16] + e[7] * e[13] * e[25] + e[1] * e[18] * e[12] + e[1] * e[9] * e[21];
			A[107] = e[10] * e[27] * e[12] + e[10] * e[9] * e[30] + e[10] * e[29] * e[14] + e[10] * e[11] * e[32] + e[10] * e[28] * e[13] + e[28] * e[11] * e[14] + e[28] * e[9] * e[12] + e[13] * e[30] * e[12] + e[13] * e[32] * e[14] + e[16] * e[30] * e[15] + e[16] * e[12] * e[33] + e[16] * e[32] * e[17] + e[16] * e[14] * e[35] + e[16] * e[13] * e[34] + e[34] * e[14] * e[17] + e[34] * e[12] * e[15] - 1.*e[13] * e[27] * e[9] - 1.*e[13] * e[33] * e[15] - 1.*e[13] * e[35] * e[17] - 1.*e[13] * e[29] * e[11] + .5000000000*ep2[10] * e[31] + .5000000000*e[31] * ep2[16] - .5000000000*e[31] * ep2[9] - .5000000000*e[31] * ep2[11] + .5000000000*e[31] * ep2[12] - .5000000000*e[31] * ep2[15] - .5000000000*e[31] * ep2[17] + .5000000000*e[31] * ep2[14] + 1.500000000*e[31] * ep2[13];
			A[106] = -.5000000000*e[4] * ep2[6] - .5000000000*e[4] * ep2[0] + e[1] * e[2] * e[5] + .5000000000*e[4] * ep2[7] + e[1] * e[0] * e[3] + e[7] * e[5] * e[8] - .5000000000*e[4] * ep2[8] + .5000000000*e[4] * ep2[3] + .5000000000*e[4] * ep2[5] + e[7] * e[3] * e[6] - .5000000000*e[4] * ep2[2] + .5000000000*ep3[4] + .5000000000*ep2[1] * e[4];
			A[100] = e[34] * e[32] * e[35] - .5000000000*e[31] * ep2[35] + .5000000000*e[31] * ep2[34] + .5000000000*ep2[28] * e[31] + .5000000000*ep3[31] + .5000000000*e[31] * ep2[32] + e[34] * e[30] * e[33] - .5000000000*e[31] * ep2[27] + .5000000000*e[31] * ep2[30] - .5000000000*e[31] * ep2[33] - .5000000000*e[31] * ep2[29] + e[28] * e[29] * e[32] + e[28] * e[27] * e[30];
			A[101] = e[1] * e[27] * e[30] + e[1] * e[29] * e[32] + e[1] * e[28] * e[31] + e[31] * e[30] * e[3] + e[31] * e[32] * e[5] + e[7] * e[30] * e[33] + e[7] * e[32] * e[35] + e[7] * e[31] * e[34] + e[28] * e[27] * e[3] + e[28] * e[0] * e[30] + e[28] * e[29] * e[5] + e[28] * e[2] * e[32] + e[34] * e[30] * e[6] + e[34] * e[3] * e[33] + e[34] * e[32] * e[8] + e[34] * e[5] * e[35] - 1.*e[31] * e[27] * e[0] - 1.*e[31] * e[33] * e[6] - 1.*e[31] * e[35] * e[8] - 1.*e[31] * e[29] * e[2] + .5000000000*e[4] * ep2[30] + .5000000000*e[4] * ep2[32] + 1.500000000*e[4] * ep2[31] - .5000000000*e[4] * ep2[27] + .5000000000*e[4] * ep2[28] - .5000000000*e[4] * ep2[29] - .5000000000*e[4] * ep2[33] + .5000000000*e[4] * ep2[34] - .5000000000*e[4] * ep2[35];
			A[102] = .5000000000*e[22] * ep2[30] + .5000000000*e[22] * ep2[32] + 1.500000000*e[22] * ep2[31] + .5000000000*e[22] * ep2[34] - .5000000000*e[22] * ep2[27] - .5000000000*e[22] * ep2[29] - .5000000000*e[22] * ep2[33] - .5000000000*e[22] * ep2[35] + e[28] * e[18] * e[30] + e[28] * e[29] * e[23] + e[28] * e[20] * e[32] + e[31] * e[30] * e[21] + e[31] * e[32] * e[23] + e[25] * e[30] * e[33] + e[25] * e[32] * e[35] + e[25] * e[31] * e[34] + e[34] * e[30] * e[24] + e[34] * e[21] * e[33] + e[34] * e[32] * e[26] + e[34] * e[23] * e[35] - 1.*e[31] * e[27] * e[18] - 1.*e[31] * e[33] * e[24] - 1.*e[31] * e[29] * e[20] - 1.*e[31] * e[35] * e[26] + e[19] * e[27] * e[30] + e[19] * e[29] * e[32] + e[19] * e[28] * e[31] + e[28] * e[27] * e[21] + .5000000000*ep2[28] * e[22];
			A[103] = e[16] * e[30] * e[33] + e[16] * e[32] * e[35] + e[10] * e[27] * e[30] + e[10] * e[29] * e[32] + e[10] * e[28] * e[31] + e[34] * e[30] * e[15] + e[34] * e[12] * e[33] + e[34] * e[32] * e[17] + e[34] * e[14] * e[35] + e[34] * e[31] * e[16] + e[28] * e[27] * e[12] + e[28] * e[9] * e[30] + e[28] * e[29] * e[14] + e[28] * e[11] * e[32] - 1.*e[31] * e[27] * e[9] + e[31] * e[30] * e[12] + e[31] * e[32] * e[14] - 1.*e[31] * e[33] * e[15] - 1.*e[31] * e[35] * e[17] - 1.*e[31] * e[29] * e[11] - .5000000000*e[13] * ep2[27] + .5000000000*e[13] * ep2[32] + .5000000000*e[13] * ep2[28] - .5000000000*e[13] * ep2[29] + 1.500000000*e[13] * ep2[31] - .5000000000*e[13] * ep2[33] + .5000000000*e[13] * ep2[30] + .5000000000*e[13] * ep2[34] - .5000000000*e[13] * ep2[35];
			A[96] = e[21] * e[23] * e[14] + e[21] * e[22] * e[13] + e[24] * e[21] * e[15] + e[24] * e[23] * e[17] + e[24] * e[14] * e[26] + e[24] * e[22] * e[16] + e[24] * e[13] * e[25] + e[15] * e[22] * e[25] + e[15] * e[23] * e[26] + e[9] * e[19] * e[22] + e[9] * e[18] * e[21] + e[9] * e[20] * e[23] + e[18] * e[20] * e[14] + e[18] * e[11] * e[23] + e[18] * e[19] * e[13] + e[18] * e[10] * e[22] - 1.*e[21] * e[25] * e[16] - 1.*e[21] * e[26] * e[17] - 1.*e[21] * e[20] * e[11] - 1.*e[21] * e[19] * e[10] + 1.500000000*ep2[21] * e[12] + .5000000000*e[12] * ep2[24] - .5000000000*e[12] * ep2[26] + .5000000000*e[12] * ep2[18] + .5000000000*e[12] * ep2[23] - .5000000000*e[12] * ep2[19] - .5000000000*e[12] * ep2[20] + .5000000000*e[12] * ep2[22] - .5000000000*e[12] * ep2[25];
			A[97] = -1.*e[12] * e[29] * e[20] - 1.*e[12] * e[35] * e[26] - 1.*e[12] * e[28] * e[19] - 1.*e[12] * e[34] * e[25] + e[18] * e[29] * e[14] + e[18] * e[11] * e[32] + e[18] * e[28] * e[13] + e[18] * e[10] * e[31] + e[27] * e[20] * e[14] + e[27] * e[11] * e[23] + e[27] * e[19] * e[13] + e[27] * e[10] * e[22] + e[15] * e[30] * e[24] + e[15] * e[21] * e[33] + e[15] * e[31] * e[25] + e[15] * e[22] * e[34] + e[15] * e[32] * e[26] + e[15] * e[23] * e[35] - 1.*e[21] * e[28] * e[10] - 1.*e[21] * e[34] * e[16] - 1.*e[21] * e[35] * e[17] - 1.*e[21] * e[29] * e[11] - 1.*e[30] * e[25] * e[16] - 1.*e[30] * e[26] * e[17] - 1.*e[30] * e[20] * e[11] - 1.*e[30] * e[19] * e[10] + e[24] * e[32] * e[17] + e[24] * e[14] * e[35] + e[24] * e[31] * e[16] + e[24] * e[13] * e[34] + e[33] * e[23] * e[17] + e[33] * e[14] * e[26] + e[33] * e[22] * e[16] + e[33] * e[13] * e[25] + 3.*e[12] * e[30] * e[21] + e[12] * e[31] * e[22] + e[12] * e[32] * e[23] + e[9] * e[27] * e[21] + e[9] * e[18] * e[30] + e[9] * e[28] * e[22] + e[9] * e[19] * e[31] + e[9] * e[29] * e[23] + e[9] * e[20] * e[32] + e[21] * e[32] * e[14] + e[21] * e[31] * e[13] + e[30] * e[23] * e[14] + e[30] * e[22] * e[13] + e[12] * e[27] * e[18] + e[12] * e[33] * e[24];
			A[98] = e[0] * e[11] * e[5] + e[0] * e[2] * e[14] + e[9] * e[1] * e[4] + e[9] * e[0] * e[3] + e[9] * e[2] * e[5] + e[3] * e[13] * e[4] + e[3] * e[14] * e[5] + e[6] * e[3] * e[15] + e[6] * e[13] * e[7] + e[6] * e[4] * e[16] + e[6] * e[14] * e[8] + e[6] * e[5] * e[17] + e[15] * e[4] * e[7] + e[15] * e[5] * e[8] - 1.*e[3] * e[11] * e[2] - 1.*e[3] * e[10] * e[1] - 1.*e[3] * e[16] * e[7] - 1.*e[3] * e[17] * e[8] + e[0] * e[10] * e[4] + e[0] * e[1] * e[13] + 1.500000000*e[12] * ep2[3] + .5000000000*e[12] * ep2[4] + .5000000000*e[12] * ep2[5] + .5000000000*e[12] * ep2[6] + .5000000000*ep2[0] * e[12] - .5000000000*e[12] * ep2[1] - .5000000000*e[12] * ep2[7] - .5000000000*e[12] * ep2[2] - .5000000000*e[12] * ep2[8];
			A[99] = e[21] * e[24] * e[6] + e[0] * e[19] * e[22] + e[0] * e[20] * e[23] + e[24] * e[22] * e[7] + e[24] * e[4] * e[25] + e[24] * e[23] * e[8] + e[24] * e[5] * e[26] + e[6] * e[22] * e[25] + e[6] * e[23] * e[26] + e[18] * e[0] * e[21] + e[18] * e[19] * e[4] + e[18] * e[1] * e[22] + e[18] * e[20] * e[5] + e[18] * e[2] * e[23] + e[21] * e[22] * e[4] + e[21] * e[23] * e[5] - 1.*e[21] * e[26] * e[8] - 1.*e[21] * e[20] * e[2] - 1.*e[21] * e[19] * e[1] - 1.*e[21] * e[25] * e[7] + 1.500000000*ep2[21] * e[3] + .5000000000*e[3] * ep2[22] + .5000000000*e[3] * ep2[23] + .5000000000*e[3] * ep2[24] - .5000000000*e[3] * ep2[26] - .5000000000*e[3] * ep2[19] - .5000000000*e[3] * ep2[20] - .5000000000*e[3] * ep2[25] + .5000000000*ep2[18] * e[3];
			A[127] = e[11] * e[27] * e[12] + e[11] * e[9] * e[30] + e[11] * e[29] * e[14] + e[11] * e[28] * e[13] + e[11] * e[10] * e[31] + e[29] * e[9] * e[12] + e[29] * e[10] * e[13] + e[14] * e[30] * e[12] + e[14] * e[31] * e[13] + e[17] * e[30] * e[15] + e[17] * e[12] * e[33] + e[17] * e[14] * e[35] + e[17] * e[31] * e[16] + e[17] * e[13] * e[34] + e[35] * e[12] * e[15] + e[35] * e[13] * e[16] - 1.*e[14] * e[27] * e[9] - 1.*e[14] * e[28] * e[10] - 1.*e[14] * e[33] * e[15] - 1.*e[14] * e[34] * e[16] + .5000000000*ep2[11] * e[32] - .5000000000*e[32] * ep2[16] - .5000000000*e[32] * ep2[9] + .5000000000*e[32] * ep2[12] - .5000000000*e[32] * ep2[15] + .5000000000*e[32] * ep2[17] - .5000000000*e[32] * ep2[10] + 1.500000000*e[32] * ep2[14] + .5000000000*e[32] * ep2[13];
			A[126] = e[8] * e[3] * e[6] + .5000000000*ep2[2] * e[5] - .5000000000*e[5] * ep2[0] + .5000000000*e[5] * ep2[4] - .5000000000*e[5] * ep2[6] + .5000000000*e[5] * ep2[8] + e[8] * e[4] * e[7] + .5000000000*ep3[5] + e[2] * e[0] * e[3] + .5000000000*e[5] * ep2[3] - .5000000000*e[5] * ep2[7] + e[2] * e[1] * e[4] - .5000000000*e[5] * ep2[1];
			A[125] = e[2] * e[27] * e[3] + e[2] * e[0] * e[30] + e[2] * e[28] * e[4] + e[2] * e[1] * e[31] + e[2] * e[29] * e[5] - 1.*e[5] * e[27] * e[0] - 1.*e[5] * e[34] * e[7] - 1.*e[5] * e[33] * e[6] + e[5] * e[30] * e[3] + e[5] * e[35] * e[8] - 1.*e[5] * e[28] * e[1] + e[5] * e[31] * e[4] + e[29] * e[1] * e[4] + e[29] * e[0] * e[3] + e[8] * e[30] * e[6] + e[8] * e[3] * e[33] + e[8] * e[31] * e[7] + e[8] * e[4] * e[34] + e[35] * e[4] * e[7] + e[35] * e[3] * e[6] + .5000000000*ep2[2] * e[32] + 1.500000000*e[32] * ep2[5] + .5000000000*e[32] * ep2[4] - .5000000000*e[32] * ep2[0] - .5000000000*e[32] * ep2[6] - .5000000000*e[32] * ep2[1] - .5000000000*e[32] * ep2[7] + .5000000000*e[32] * ep2[3] + .5000000000*e[32] * ep2[8];
			A[124] = -1.*e[14] * e[19] * e[1] + e[14] * e[22] * e[4] - 1.*e[14] * e[18] * e[0] - 1.*e[14] * e[25] * e[7] - 1.*e[14] * e[24] * e[6] - 1.*e[23] * e[10] * e[1] + e[23] * e[13] * e[4] - 1.*e[23] * e[16] * e[7] - 1.*e[23] * e[15] * e[6] - 1.*e[23] * e[9] * e[0] + e[23] * e[12] * e[3] + e[17] * e[21] * e[6] + e[17] * e[3] * e[24] + e[17] * e[22] * e[7] + e[17] * e[4] * e[25] + e[17] * e[5] * e[26] - 1.*e[5] * e[24] * e[15] - 1.*e[5] * e[25] * e[16] - 1.*e[5] * e[18] * e[9] - 1.*e[5] * e[19] * e[10] + e[26] * e[12] * e[6] + e[26] * e[3] * e[15] + e[26] * e[13] * e[7] + e[26] * e[4] * e[16] + e[11] * e[18] * e[3] + e[11] * e[0] * e[21] + e[11] * e[19] * e[4] + e[11] * e[1] * e[22] + e[11] * e[20] * e[5] + e[11] * e[2] * e[23] + e[20] * e[9] * e[3] + e[20] * e[0] * e[12] + e[20] * e[10] * e[4] + e[20] * e[1] * e[13] + e[20] * e[2] * e[14] + e[5] * e[21] * e[12] + 3.*e[5] * e[23] * e[14] + e[5] * e[22] * e[13] + e[8] * e[21] * e[15] + e[8] * e[12] * e[24] + e[8] * e[23] * e[17] + e[8] * e[14] * e[26] + e[8] * e[22] * e[16] + e[8] * e[13] * e[25] + e[2] * e[18] * e[12] + e[2] * e[9] * e[21] + e[2] * e[19] * e[13] + e[2] * e[10] * e[22] + e[14] * e[21] * e[3];
			A[123] = -.5000000000*e[14] * ep2[27] + 1.500000000*e[14] * ep2[32] - .5000000000*e[14] * ep2[28] + .5000000000*e[14] * ep2[29] + .5000000000*e[14] * ep2[31] - .5000000000*e[14] * ep2[33] + .5000000000*e[14] * ep2[30] - .5000000000*e[14] * ep2[34] + .5000000000*e[14] * ep2[35] + e[11] * e[27] * e[30] + e[11] * e[29] * e[32] + e[11] * e[28] * e[31] + e[35] * e[30] * e[15] + e[35] * e[12] * e[33] + e[35] * e[32] * e[17] + e[35] * e[31] * e[16] + e[35] * e[13] * e[34] + e[29] * e[27] * e[12] + e[29] * e[9] * e[30] + e[29] * e[28] * e[13] + e[29] * e[10] * e[31] - 1.*e[32] * e[27] * e[9] + e[32] * e[30] * e[12] - 1.*e[32] * e[28] * e[10] + e[32] * e[31] * e[13] - 1.*e[32] * e[33] * e[15] - 1.*e[32] * e[34] * e[16] + e[17] * e[30] * e[33] + e[17] * e[31] * e[34];
			A[122] = -.5000000000*e[23] * ep2[33] - .5000000000*e[23] * ep2[34] + .5000000000*ep2[29] * e[23] + .5000000000*e[23] * ep2[30] + 1.500000000*e[23] * ep2[32] + .5000000000*e[23] * ep2[31] + .5000000000*e[23] * ep2[35] - .5000000000*e[23] * ep2[27] - .5000000000*e[23] * ep2[28] + e[32] * e[30] * e[21] + e[32] * e[31] * e[22] + e[26] * e[30] * e[33] + e[26] * e[32] * e[35] + e[26] * e[31] * e[34] + e[35] * e[30] * e[24] + e[35] * e[21] * e[33] + e[35] * e[31] * e[25] + e[35] * e[22] * e[34] - 1.*e[32] * e[27] * e[18] - 1.*e[32] * e[33] * e[24] - 1.*e[32] * e[28] * e[19] - 1.*e[32] * e[34] * e[25] + e[20] * e[27] * e[30] + e[20] * e[29] * e[32] + e[20] * e[28] * e[31] + e[29] * e[27] * e[21] + e[29] * e[18] * e[30] + e[29] * e[28] * e[22] + e[29] * e[19] * e[31];
			A[121] = e[2] * e[27] * e[30] + e[2] * e[29] * e[32] + e[2] * e[28] * e[31] + e[32] * e[30] * e[3] + e[32] * e[31] * e[4] + e[8] * e[30] * e[33] + e[8] * e[32] * e[35] + e[8] * e[31] * e[34] + e[29] * e[27] * e[3] + e[29] * e[0] * e[30] + e[29] * e[28] * e[4] + e[29] * e[1] * e[31] + e[35] * e[30] * e[6] + e[35] * e[3] * e[33] + e[35] * e[31] * e[7] + e[35] * e[4] * e[34] - 1.*e[32] * e[27] * e[0] - 1.*e[32] * e[34] * e[7] - 1.*e[32] * e[33] * e[6] - 1.*e[32] * e[28] * e[1] + .5000000000*e[5] * ep2[30] + 1.500000000*e[5] * ep2[32] + .5000000000*e[5] * ep2[31] - .5000000000*e[5] * ep2[27] - .5000000000*e[5] * ep2[28] + .5000000000*e[5] * ep2[29] - .5000000000*e[5] * ep2[33] - .5000000000*e[5] * ep2[34] + .5000000000*e[5] * ep2[35];
			A[120] = .5000000000*e[32] * ep2[31] + .5000000000*e[32] * ep2[35] - .5000000000*e[32] * ep2[27] + e[29] * e[27] * e[30] + e[29] * e[28] * e[31] + e[35] * e[30] * e[33] + e[35] * e[31] * e[34] + .5000000000*ep2[29] * e[32] + .5000000000*ep3[32] - .5000000000*e[32] * ep2[33] - .5000000000*e[32] * ep2[34] + .5000000000*e[32] * ep2[30] - .5000000000*e[32] * ep2[28];
			A[118] = e[10] * e[1] * e[4] + e[10] * e[0] * e[3] + e[10] * e[2] * e[5] + e[4] * e[12] * e[3] + e[4] * e[14] * e[5] + e[7] * e[12] * e[6] + e[7] * e[3] * e[15] + e[7] * e[4] * e[16] + e[7] * e[14] * e[8] + e[7] * e[5] * e[17] + e[16] * e[3] * e[6] + e[16] * e[5] * e[8] - 1.*e[4] * e[11] * e[2] - 1.*e[4] * e[15] * e[6] - 1.*e[4] * e[9] * e[0] - 1.*e[4] * e[17] * e[8] + e[1] * e[9] * e[3] + e[1] * e[0] * e[12] + e[1] * e[11] * e[5] + e[1] * e[2] * e[14] + 1.500000000*e[13] * ep2[4] + .5000000000*e[13] * ep2[3] + .5000000000*e[13] * ep2[5] + .5000000000*e[13] * ep2[7] + .5000000000*ep2[1] * e[13] - .5000000000*e[13] * ep2[0] - .5000000000*e[13] * ep2[6] - .5000000000*e[13] * ep2[2] - .5000000000*e[13] * ep2[8];
			A[119] = e[25] * e[21] * e[6] + e[25] * e[3] * e[24] + e[25] * e[23] * e[8] + e[25] * e[5] * e[26] + e[7] * e[21] * e[24] + e[7] * e[23] * e[26] + e[19] * e[18] * e[3] + e[19] * e[0] * e[21] + e[19] * e[1] * e[22] + e[19] * e[20] * e[5] + e[19] * e[2] * e[23] + e[22] * e[21] * e[3] + e[22] * e[23] * e[5] - 1.*e[22] * e[26] * e[8] - 1.*e[22] * e[20] * e[2] - 1.*e[22] * e[18] * e[0] + e[22] * e[25] * e[7] - 1.*e[22] * e[24] * e[6] + e[1] * e[18] * e[21] + e[1] * e[20] * e[23] + .5000000000*e[4] * ep2[25] - .5000000000*e[4] * ep2[26] - .5000000000*e[4] * ep2[18] - .5000000000*e[4] * ep2[20] - .5000000000*e[4] * ep2[24] + .5000000000*ep2[19] * e[4] + 1.500000000*ep2[22] * e[4] + .5000000000*e[4] * ep2[21] + .5000000000*e[4] * ep2[23];
			A[116] = e[22] * e[21] * e[12] + e[22] * e[23] * e[14] + e[25] * e[21] * e[15] + e[25] * e[12] * e[24] + e[25] * e[23] * e[17] + e[25] * e[14] * e[26] + e[25] * e[22] * e[16] + e[16] * e[21] * e[24] + e[16] * e[23] * e[26] + e[10] * e[19] * e[22] + e[10] * e[18] * e[21] + e[10] * e[20] * e[23] + e[19] * e[18] * e[12] + e[19] * e[9] * e[21] + e[19] * e[20] * e[14] + e[19] * e[11] * e[23] - 1.*e[22] * e[24] * e[15] - 1.*e[22] * e[26] * e[17] - 1.*e[22] * e[20] * e[11] - 1.*e[22] * e[18] * e[9] - .5000000000*e[13] * ep2[26] - .5000000000*e[13] * ep2[18] + .5000000000*e[13] * ep2[23] + .5000000000*e[13] * ep2[19] - .5000000000*e[13] * ep2[20] - .5000000000*e[13] * ep2[24] + .5000000000*e[13] * ep2[21] + 1.500000000*ep2[22] * e[13] + .5000000000*e[13] * ep2[25];
			A[117] = e[13] * e[30] * e[21] + 3.*e[13] * e[31] * e[22] + e[13] * e[32] * e[23] + e[10] * e[27] * e[21] + e[10] * e[18] * e[30] + e[10] * e[28] * e[22] + e[10] * e[19] * e[31] + e[10] * e[29] * e[23] + e[10] * e[20] * e[32] + e[22] * e[30] * e[12] + e[22] * e[32] * e[14] + e[31] * e[21] * e[12] + e[31] * e[23] * e[14] - 1.*e[13] * e[27] * e[18] - 1.*e[13] * e[33] * e[24] - 1.*e[13] * e[29] * e[20] - 1.*e[13] * e[35] * e[26] + e[13] * e[28] * e[19] + e[13] * e[34] * e[25] + e[19] * e[27] * e[12] + e[19] * e[9] * e[30] + e[19] * e[29] * e[14] + e[19] * e[11] * e[32] + e[28] * e[18] * e[12] + e[28] * e[9] * e[21] + e[28] * e[20] * e[14] + e[28] * e[11] * e[23] + e[16] * e[30] * e[24] + e[16] * e[21] * e[33] + e[16] * e[31] * e[25] + e[16] * e[22] * e[34] + e[16] * e[32] * e[26] + e[16] * e[23] * e[35] - 1.*e[22] * e[27] * e[9] - 1.*e[22] * e[33] * e[15] - 1.*e[22] * e[35] * e[17] - 1.*e[22] * e[29] * e[11] - 1.*e[31] * e[24] * e[15] - 1.*e[31] * e[26] * e[17] - 1.*e[31] * e[20] * e[11] - 1.*e[31] * e[18] * e[9] + e[25] * e[30] * e[15] + e[25] * e[12] * e[33] + e[25] * e[32] * e[17] + e[25] * e[14] * e[35] + e[34] * e[21] * e[15] + e[34] * e[12] * e[24] + e[34] * e[23] * e[17] + e[34] * e[14] * e[26];
			A[114] = e[19] * e[11] * e[14] + e[19] * e[9] * e[12] + e[19] * e[10] * e[13] + e[13] * e[21] * e[12] + e[13] * e[23] * e[14] + e[16] * e[21] * e[15] + e[16] * e[12] * e[24] + e[16] * e[23] * e[17] + e[16] * e[14] * e[26] + e[16] * e[13] * e[25] + e[25] * e[14] * e[17] + e[25] * e[12] * e[15] - 1.*e[13] * e[24] * e[15] - 1.*e[13] * e[26] * e[17] - 1.*e[13] * e[20] * e[11] - 1.*e[13] * e[18] * e[9] + e[10] * e[18] * e[12] + e[10] * e[9] * e[21] + e[10] * e[20] * e[14] + e[10] * e[11] * e[23] + 1.500000000*e[22] * ep2[13] + .5000000000*e[22] * ep2[14] + .5000000000*e[22] * ep2[12] + .5000000000*e[22] * ep2[16] + .5000000000*ep2[10] * e[22] - .5000000000*e[22] * ep2[9] - .5000000000*e[22] * ep2[11] - .5000000000*e[22] * ep2[15] - .5000000000*e[22] * ep2[17];
			A[115] = e[13] * e[12] * e[3] + e[13] * e[14] * e[5] + e[16] * e[12] * e[6] + e[16] * e[3] * e[15] + e[16] * e[13] * e[7] + e[16] * e[14] * e[8] + e[16] * e[5] * e[17] + e[7] * e[14] * e[17] + e[7] * e[12] * e[15] + e[1] * e[11] * e[14] + e[1] * e[9] * e[12] + e[1] * e[10] * e[13] + e[10] * e[9] * e[3] + e[10] * e[0] * e[12] + e[10] * e[11] * e[5] + e[10] * e[2] * e[14] - 1.*e[13] * e[11] * e[2] - 1.*e[13] * e[15] * e[6] - 1.*e[13] * e[9] * e[0] - 1.*e[13] * e[17] * e[8] + 1.500000000*ep2[13] * e[4] + .5000000000*e[4] * ep2[16] - .5000000000*e[4] * ep2[9] - .5000000000*e[4] * ep2[11] + .5000000000*e[4] * ep2[12] - .5000000000*e[4] * ep2[15] - .5000000000*e[4] * ep2[17] + .5000000000*e[4] * ep2[10] + .5000000000*e[4] * ep2[14];
			A[112] = e[19] * e[1] * e[4] + e[19] * e[0] * e[3] + e[19] * e[2] * e[5] + e[4] * e[21] * e[3] + e[4] * e[23] * e[5] + e[7] * e[21] * e[6] + e[7] * e[3] * e[24] + e[7] * e[4] * e[25] + e[7] * e[23] * e[8] + e[7] * e[5] * e[26] + e[25] * e[3] * e[6] + e[25] * e[5] * e[8] + e[1] * e[18] * e[3] + e[1] * e[0] * e[21] + e[1] * e[20] * e[5] + e[1] * e[2] * e[23] - 1.*e[4] * e[26] * e[8] - 1.*e[4] * e[20] * e[2] - 1.*e[4] * e[18] * e[0] - 1.*e[4] * e[24] * e[6] + 1.500000000*e[22] * ep2[4] - .5000000000*e[22] * ep2[0] - .5000000000*e[22] * ep2[6] + .5000000000*e[22] * ep2[5] + .5000000000*e[22] * ep2[1] + .5000000000*e[22] * ep2[7] + .5000000000*e[22] * ep2[3] - .5000000000*e[22] * ep2[2] - .5000000000*e[22] * ep2[8];
			A[113] = -1.*e[31] * e[20] * e[2] - 1.*e[31] * e[18] * e[0] + e[31] * e[23] * e[5] - 1.*e[31] * e[24] * e[6] + e[7] * e[30] * e[24] + e[7] * e[21] * e[33] + e[7] * e[32] * e[26] + e[7] * e[23] * e[35] + e[25] * e[30] * e[6] + e[25] * e[3] * e[33] + e[25] * e[31] * e[7] + e[25] * e[4] * e[34] + e[25] * e[32] * e[8] + e[25] * e[5] * e[35] + e[34] * e[21] * e[6] + e[34] * e[3] * e[24] + e[34] * e[22] * e[7] + e[34] * e[23] * e[8] + e[34] * e[5] * e[26] + e[1] * e[27] * e[21] + e[1] * e[18] * e[30] + e[1] * e[28] * e[22] + e[1] * e[19] * e[31] + e[1] * e[29] * e[23] + e[1] * e[20] * e[32] + e[19] * e[27] * e[3] + e[19] * e[0] * e[30] + e[19] * e[28] * e[4] + e[19] * e[29] * e[5] + e[19] * e[2] * e[32] + e[28] * e[18] * e[3] + e[28] * e[0] * e[21] + e[28] * e[20] * e[5] + e[28] * e[2] * e[23] + e[4] * e[30] * e[21] + 3.*e[4] * e[31] * e[22] + e[4] * e[32] * e[23] - 1.*e[4] * e[27] * e[18] - 1.*e[4] * e[33] * e[24] - 1.*e[4] * e[29] * e[20] - 1.*e[4] * e[35] * e[26] - 1.*e[22] * e[27] * e[0] + e[22] * e[32] * e[5] - 1.*e[22] * e[33] * e[6] + e[22] * e[30] * e[3] - 1.*e[22] * e[35] * e[8] - 1.*e[22] * e[29] * e[2] + e[31] * e[21] * e[3] - 1.*e[31] * e[26] * e[8];

			int perm[20] = { 6, 8, 18, 15, 12, 5, 14, 7, 4, 11, 19, 13, 1, 16, 17, 3, 10, 9, 2, 0 };
			double AA[200];
			for (int i = 0; i < 20; i++)
			{
				for (int j = 0; j < 10; j++) AA[i + j * 20] = A[perm[i] + j * 20];
			}

			for (int i = 0; i < 200; i++)
			{
				A[i] = AA[i];
			}
		}


		void computeError(InputArray _m1, InputArray _m2, InputArray _model, OutputArray _err) const
		{
			Mat X1 = _m1.getMat(), X2 = _m2.getMat(), model = _model.getMat();
			const Point2d* x1ptr = X1.ptr<Point2d>();
			const Point2d* x2ptr = X2.ptr<Point2d>();
			int n = X1.checkVector(2);
			Matx33d E(model.ptr<double>());

			_err.create(n, 1, CV_32F);
			Mat err = _err.getMat();

			for (int i = 0; i < n; i++)
			{
				Vec3d x1(x1ptr[i].x, x1ptr[i].y, 1.);
				Vec3d x2(x2ptr[i].x, x2ptr[i].y, 1.);
				Vec3d Ex1 = E * x1;
				Vec3d Etx2 = E.t() * x2;
				double x2tEx1 = x2.dot(Ex1);

				double a = Ex1[0] * Ex1[0];
				double b = Ex1[1] * Ex1[1];
				double c = Etx2[0] * Etx2[0];
				double d = Etx2[1] * Etx2[1];

				err.at<float>(i) = (float)(x2tEx1 * x2tEx1 / (a + b + c + d));
			}
		}
	};

}

namespace FP
{
	// MF: copied from calib3d.hpp from OpenCV 3.0

	//! finds essential matrix from a set of corresponding 2D points using five-point algorithm
	// Input should be a vector of n 2D points or a Nx2 matrix
	static cv::Mat findEssentialMat(cv::InputArray _points1, cv::InputArray _points2, double focal, cv::Point2d pp,
		int method, double prob, double threshold, cv::OutputArray _mask)
	{
		cv::Mat points1, points2;
		_points1.getMat().convertTo(points1, CV_64F);
		_points2.getMat().convertTo(points2, CV_64F);

		int npoints = points1.checkVector(2);
		CV_Assert(npoints >= 5 && points2.checkVector(2) == npoints &&
			points1.type() == points2.type());

		if (points1.channels() > 1)
		{
			points1 = points1.reshape(1, npoints);
			points2 = points2.reshape(1, npoints);
		}

		double ifocal = focal != 0 ? 1. / focal : 1.;
		for (int i = 0; i < npoints; i++)
		{
			points1.at<double>(i, 0) = (points1.at<double>(i, 0) - pp.x)*ifocal;
			points1.at<double>(i, 1) = (points1.at<double>(i, 1) - pp.y)*ifocal;
			points2.at<double>(i, 0) = (points2.at<double>(i, 0) - pp.x)*ifocal;
			points2.at<double>(i, 1) = (points2.at<double>(i, 1) - pp.y)*ifocal;
		}

		// Reshape data to fit opencv ransac function
		points1 = points1.reshape(2, npoints);
		points2 = points2.reshape(2, npoints);

		threshold /= focal;

		cv::Mat E;
		if (method == cv::RANSAC)
		{
			cv::EMEstimatorCallback ec;
			ec.DoTheThing(5, threshold, prob, points1, points2, E, _mask);
			//cv::createRANSACPointSetRegistrator(cv::makePtr<cv::EMEstimatorCallback>(), 5, threshold, prob)->run(points1, points2, E, _mask);
		}

			



		return E;
	}
	
	//! decompose essential matrix to possible rotation matrix and one translation vector
	static void decomposeEssentialMat(cv::InputArray _E, cv::OutputArray _R1, cv::OutputArray _R2, cv::OutputArray _t)
	{
		cv::Mat E = _E.getMat().reshape(1, 3);
		CV_Assert(E.cols == 3 && E.rows == 3);

		cv::Mat D, U, Vt;
		cv::SVD::compute(E, D, U, Vt);

		if (cv::determinant(U) < 0) U *= -1.;
		if (cv::determinant(Vt) < 0) Vt *= -1.;

		cv::Mat W = (cv::Mat_<double>(3, 3) << 0, 1, 0, -1, 0, 0, 0, 0, 1);
		W.convertTo(W, E.type());

		cv::Mat R1, R2, t;
		R1 = U * W * Vt;
		R2 = U * W.t() * Vt;
		t = U.col(2) * 1.0;

		R1.copyTo(_R1);
		R2.copyTo(_R2);
		t.copyTo(_t);
	}

	//! recover relative camera pose from a set of corresponding 2D points
	int recoverPose(cv::InputArray E, cv::InputArray _points1, cv::InputArray _points2, cv::OutputArray _R, cv::OutputArray _t, double focal, cv::Point2d pp, cv::InputOutputArray _mask)
	{
		cv::Mat points1, points2;
		_points1.getMat().copyTo(points1);
		_points2.getMat().copyTo(points2);

		int npoints = points1.checkVector(2);
		CV_Assert(npoints >= 0 && points2.checkVector(2) == npoints &&
			points1.type() == points2.type());

		if (points1.channels() > 1)
		{
			points1 = points1.reshape(1, npoints);
			points2 = points2.reshape(1, npoints);
		}
		points1.convertTo(points1, CV_64F);
		points2.convertTo(points2, CV_64F);

		points1.col(0) = (points1.col(0) - pp.x) / focal;
		points2.col(0) = (points2.col(0) - pp.x) / focal;
		points1.col(1) = (points1.col(1) - pp.y) / focal;
		points2.col(1) = (points2.col(1) - pp.y) / focal;

		points1 = points1.t();
		points2 = points2.t();

		cv::Mat R1, R2, t;
		FP::decomposeEssentialMat(E, R1, R2, t);
		cv::Mat P0 = cv::Mat::eye(3, 4, R1.type());
		cv::Mat P1(3, 4, R1.type()), P2(3, 4, R1.type()), P3(3, 4, R1.type()), P4(3, 4, R1.type());
		P1(cv::Range::all(), cv::Range(0, 3)) = R1 * 1.0; P1.col(3) = t * 1.0;
		P2(cv::Range::all(), cv::Range(0, 3)) = R2 * 1.0; P2.col(3) = t * 1.0;
		P3(cv::Range::all(), cv::Range(0, 3)) = R1 * 1.0; P3.col(3) = -t * 1.0;
		P4(cv::Range::all(), cv::Range(0, 3)) = R2 * 1.0; P4.col(3) = -t * 1.0;

		// Do the cheirality check.
		// Notice here a threshold dist is used to filter
		// out far away points (i.e. infinite points) since
		// there depth may vary between postive and negtive.
		double dist = 50.0;
		cv::Mat Q;
		cv::triangulatePoints(P0, P1, points1, points2, Q);
		cv::Mat mask1 = Q.row(2).mul(Q.row(3)) > 0;
		Q.row(0) /= Q.row(3);
		Q.row(1) /= Q.row(3);
		Q.row(2) /= Q.row(3);
		Q.row(3) /= Q.row(3);
		mask1 = (Q.row(2) < dist) & mask1;
		Q = P1 * Q;
		mask1 = (Q.row(2) > 0) & mask1;
		mask1 = (Q.row(2) < dist) & mask1;

		cv::triangulatePoints(P0, P2, points1, points2, Q);
		cv::Mat mask2 = Q.row(2).mul(Q.row(3)) > 0;
		Q.row(0) /= Q.row(3);
		Q.row(1) /= Q.row(3);
		Q.row(2) /= Q.row(3);
		Q.row(3) /= Q.row(3);
		mask2 = (Q.row(2) < dist) & mask2;
		Q = P2 * Q;
		mask2 = (Q.row(2) > 0) & mask2;
		mask2 = (Q.row(2) < dist) & mask2;

		triangulatePoints(P0, P3, points1, points2, Q);
		cv::Mat mask3 = Q.row(2).mul(Q.row(3)) > 0;
		Q.row(0) /= Q.row(3);
		Q.row(1) /= Q.row(3);
		Q.row(2) /= Q.row(3);
		Q.row(3) /= Q.row(3);
		mask3 = (Q.row(2) < dist) & mask3;
		Q = P3 * Q;
		mask3 = (Q.row(2) > 0) & mask3;
		mask3 = (Q.row(2) < dist) & mask3;

		triangulatePoints(P0, P4, points1, points2, Q);
		cv::Mat mask4 = Q.row(2).mul(Q.row(3)) > 0;
		Q.row(0) /= Q.row(3);
		Q.row(1) /= Q.row(3);
		Q.row(2) /= Q.row(3);
		Q.row(3) /= Q.row(3);
		mask4 = (Q.row(2) < dist) & mask4;
		Q = P4 * Q;
		mask4 = (Q.row(2) > 0) & mask4;
		mask4 = (Q.row(2) < dist) & mask4;

		mask1 = mask1.t();
		mask2 = mask2.t();
		mask3 = mask3.t();
		mask4 = mask4.t();

		// If _mask is given, then use it to filter outliers.
		if (!_mask.empty())
		{
			cv::Mat mask = _mask.getMat();
			CV_Assert(mask.size() == mask1.size());
			bitwise_and(mask, mask1, mask1);
			bitwise_and(mask, mask2, mask2);
			bitwise_and(mask, mask3, mask3);
			bitwise_and(mask, mask4, mask4);
		}
		if (_mask.empty() && _mask.needed())
		{
			_mask.create(mask1.size(), CV_8U);
		}

		CV_Assert(_R.needed() && _t.needed());
		_R.create(3, 3, R1.type());
		_t.create(3, 1, t.type());

		int good1 = countNonZero(mask1);
		int good2 = countNonZero(mask2);
		int good3 = countNonZero(mask3);
		int good4 = countNonZero(mask4);

		if (good1 >= good2 && good1 >= good3 && good1 >= good4)
		{
			R1.copyTo(_R);
			t.copyTo(_t);
			if (_mask.needed()) mask1.copyTo(_mask);
			return good1;
		}
		else if (good2 >= good1 && good2 >= good3 && good2 >= good4)
		{
			R2.copyTo(_R);
			t.copyTo(_t);
			if (_mask.needed()) mask2.copyTo(_mask);
			return good2;
		}
		else if (good3 >= good1 && good3 >= good2 && good3 >= good4)
		{
			t = -t;
			R1.copyTo(_R);
			t.copyTo(_t);
			if (_mask.needed()) mask3.copyTo(_mask);
			return good3;
		}
		else
		{
			t = -t;
			R2.copyTo(_R);
			t.copyTo(_t);
			if (_mask.needed()) mask4.copyTo(_mask);
			return good4;
		}
	}


	// MF:
	// input:
	// matched points from two images passed as two Mats with following structure:
	//			points1:				points2:
	// row 1:	x1		y1				x1'		y1'
	// row 2:	x2		y2				x2'		y2'
	// ...		...		...				...		...
	// row n:	xn		yn				xn'		yn'
	// the size of each Mat is n rows and 2 cols (500x2)

	// threshold � Parameter used for RANSAC. It is the maximum distance from a point to an epipolar line in pixels, beyond which the point is considered an outlier and is not used for computing the final fundamental matrix. It can be set to something like 1-3, depending on the accuracy of the point localization, image resolution, and the image noise.

	// output:
	// rotation (matrix of size 3x3)
	// translation (matrix of size 3x1)

	void RotationTranslationFromFivePointAlgorithm(const cv::Mat& points1, const cv::Mat& points2, double threshold, cv::Mat& rotation, cv::Mat& translation)
	{
		cv::Mat essentialMatrixInliers;
		cv::Mat essentialMatrix = FP::findEssentialMat(points1, points2, 1.0, cv::Point2d(0, 0), cv::FM_RANSAC, 0.99, threshold, essentialMatrixInliers);

		cv::Mat R, t;
		FP::recoverPose(essentialMatrix, points1, points2, R, t, 1.0, cv::Point2d(0, 0), essentialMatrixInliers);

		rotation = R;
		translation = t;
	}


}


