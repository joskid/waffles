/*
  The contents of this file are dedicated by all of its authors, including

    Michael S. Gashler,
    Eric Moyer,
    anonymous contributors,

  to the public domain (http://creativecommons.org/publicdomain/zero/1.0/).

  Note that some moral obligations still exist in the absence of legal ones.
  For example, it would still be dishonest to deliberately misrepresent the
  origin of a work. Although we impose no legal requirements to obtain a
  license, it is beseeming for those who build on the works of others to
  give back useful improvements, or find a way to pay it forward. If
  you would like to cite us, a published paper about Waffles can be found
  at http://jmlr.org/papers/volume12/gashler11a/gashler11a.pdf. If you find
  our code to be useful, the Waffles team would love to hear how you use it.
*/

#include "GMixtureOfGaussians.h"
#include "GMatrix.h"
#include <math.h>
#include "GVec.h"
#include "GRand.h"
#include <cmath>
using namespace GClasses;
using std::vector;

GMixtureOfGaussians::GMixtureOfGaussians(int nKernelCount, GMatrix* pData, int nAttribute, double minVariance, GRand* pRand)
{
	m_nKernelCount = nKernelCount;
	m_pData = pData;
	m_nAttribute = nAttribute;
	m_dMinVariance = minVariance;
	m_pArrMeanVarWeight = new double[3 * m_nKernelCount + 4 * m_nKernelCount];
	m_pCatLikelihoods = &m_pArrMeanVarWeight[3 * m_nKernelCount];
	m_pTemp = &m_pCatLikelihoods[m_nKernelCount];
	double min = pData->columnMin(nAttribute);
	double range = pData->columnMax(nAttribute) - min;
	double d;
	int i;
	int pos = 0;
	for(i = 0; i < nKernelCount; i++)
	{
		m_pArrMeanVarWeight[pos++] = pRand->uniform() * range + min;
		d = pRand->uniform() * pRand->uniform() * (range - range / nKernelCount) + range / nKernelCount;
		m_pArrMeanVarWeight[pos++] = d * d;
		m_pArrMeanVarWeight[pos++] = 1.0 / nKernelCount;
	}
}

// virtual
GMixtureOfGaussians::~GMixtureOfGaussians()
{
	delete[] m_pArrMeanVarWeight;
}

// static
GMixtureOfGaussians* GMixtureOfGaussians::stochasticHammer(int nMinKernelCount, int nMaxKernelCount, int nItters, int nTrials, GMatrix* pData, int nAttribute, double minVariance, GRand* pRand)
{
	double dBestLogLikelihood = -1e300;
	double d;
	GMixtureOfGaussians* pBestMog = NULL;
	GMixtureOfGaussians* pMog;
	int trial, i;
	for(trial = 0; trial < nTrials; trial++)
	{
		pMog = new GMixtureOfGaussians((int)pRand->next(nMaxKernelCount - nMinKernelCount + 1) + nMinKernelCount, pData, nAttribute, minVariance, pRand);
		for(i = 1; i < nItters; i++)
			pMog->iterate();
		d = pMog->iterate();
		if(d > dBestLogLikelihood || !pBestMog)
		{
			dBestLogLikelihood = d;
			delete(pBestMog);
			pBestMog = pMog;
			pMog = NULL;
		}
		else
			delete(pMog);
	}
	return pBestMog;
}

double GMixtureOfGaussians::evalKernel(double x, int nKernel)
{
	m_dist.setMeanAndVariance(m_pArrMeanVarWeight[3 * nKernel], m_pArrMeanVarWeight[3 * nKernel + 1]);
	return m_pArrMeanVarWeight[3 * nKernel + 2] * m_dist.likelihood(x);
}

double GMixtureOfGaussians::likelihoodOfEachCategoryGivenThisFeature(double x)
{
	int i;
	double sum = 0;
	for(i = 0; i < m_nKernelCount; i++)
	{
		m_pCatLikelihoods[i] = evalKernel(x, i);
		sum += m_pCatLikelihoods[i];
	}
	if(sum > 0)
	{
		// Normalize
		for(i = 0; i < m_nKernelCount; i++)
			m_pCatLikelihoods[i] /= sum;
	}
	else
	{
		// Reset to reasonable values
		for(i = 0; i < m_nKernelCount; i++)
			m_pCatLikelihoods[i] = 1.0 / m_nKernelCount;
	}
	return sum;
}

double GMixtureOfGaussians::iterate()
{
	// Compute the maximum likelihood kernel parameters
	int i;
	double x, d;
	double likelihood = 0;
	GVecWrapper vw(m_pTemp, 3 * m_nKernelCount);
	vw.fill(0.0);
	for(size_t j = 0; j < m_pData->rows(); j++)
	{
		x = m_pData->row(j)[m_nAttribute];
		likelihood += log(likelihoodOfEachCategoryGivenThisFeature(x));
		for(i = 0; i < m_nKernelCount; i++)
		{
			m_pTemp[3 * i] += m_pCatLikelihoods[i] * x;
			d = x - m_pArrMeanVarWeight[3 * i];
			m_pTemp[3 * i + 1] += m_pCatLikelihoods[i] * d * d;
			m_pTemp[3 * i + 2] += m_pCatLikelihoods[i];
		}
	}

	// Set the new kernel parameters
	for(i = 0; i < m_nKernelCount; i++)
	{
		m_pArrMeanVarWeight[3 * i] = m_pTemp[3 * i] / m_pTemp[3 * i + 2];
		m_pArrMeanVarWeight[3 * i + 1] = m_pTemp[3 * i + 1] / m_pTemp[3 * i + 2]; //sqrt(m_pTemp[3 * i + 1] / m_pTemp[3 * i + 2]);
		if(m_pArrMeanVarWeight[3 * i + 1] < m_dMinVariance)
			m_pArrMeanVarWeight[3 * i + 1] = m_dMinVariance;
		m_pArrMeanVarWeight[3 * i + 2] = m_pTemp[3 * i + 2] / m_pData->rows();
	}

	return likelihood;
}

void GMixtureOfGaussians::params(int nKernel, double* pMean, double* pVariance, double* pWeight)
{
	*pMean = m_pArrMeanVarWeight[3 * nKernel];
	*pVariance = m_pArrMeanVarWeight[3 * nKernel + 1];
	*pWeight = m_pArrMeanVarWeight[3 * nKernel + 2];
}

#ifndef NO_TEST_CODE

#define KERNEL_COUNT 2
#define SAMPLE_COUNT 10000
#define TOLERANCE 0.8

// static
void GMixtureOfGaussians::test()
{
	// Randomly pick target weights
	GRand prng(3);
	GCategoricalDistribution cat;
	GVec& weights = cat.values(KERNEL_COUNT);
	int i;
	vector<double> probs;
	for(i = 0; i < KERNEL_COUNT; i++)
		weights[i] = prng.uniform() * .7 + .3;
	cat.normalize();
	for(i = 0; i < KERNEL_COUNT; i++)
		probs.push_back(weights[i]);

	// Randomly pick target mean and variances
	double params[2 * KERNEL_COUNT];
	for(i = 0; i < KERNEL_COUNT; i++)
	{
		params[2 * i] = prng.uniform() * 3 * KERNEL_COUNT;
		params[2 * i + 1] = prng.uniform() + 1;
	}

	// Make samples
	GNormalDistribution dists[KERNEL_COUNT];
	for(i = 0; i < KERNEL_COUNT; i++)
		dists[i].setMeanAndVariance(params[2 * i], params[2 * i + 1] * params[2 * i + 1]);
	GMatrix data(0, 1);
	for(i = 0; i < SAMPLE_COUNT; i++)
	{
		GVec& vec = data.newRow();
		size_t j = prng.categorical(probs);
		vec[0] = prng.normal() * sqrt(dists[j].variance()) + dists[j].mean();
	}

	// Approxiimate the data with a mixture of Gaussians
	GMixtureOfGaussians mog(KERNEL_COUNT, &data, 0/*attr*/, .2/*min variance*/, &prng);
	for(i = 0; i < 300; i++)
		mog.iterate();

	// Check that we got something close to the target values
	double mean, var, weight;
	for(i = 0; i < KERNEL_COUNT; i++)
	{
		mog.params(i, &mean, &var, &weight);
		size_t j;
		for(j = 0; j < KERNEL_COUNT; j++)
		{
			if(	std::abs(mean - params[2 * j]) < TOLERANCE &&
				std::abs(sqrt(var) - params[2 * j + 1]) < TOLERANCE &&
				std::abs(weight - cat.likelihood((double)j)) < TOLERANCE)
				break;
		}
		if(j >= KERNEL_COUNT)
			throw Ex("Failed");
	}
}
#endif // !NO_TEST_CODE
