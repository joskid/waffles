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

#include "GVec.h"
#include <cstdio>
#include <cstring>
#include "GRand.h"
#include "GError.h"
#include "GMatrix.h"
#ifndef MIN_PREDICT
#include "GBits.h"
#endif // MIN_PREDICT
#include "GDom.h"
#ifndef MIN_PREDICT
#include "GMath.h"
#include "GImage.h"
#endif // MIN_PREDICT
#include "GBitTable.h"
#include "GHolders.h"
#include <cmath>

namespace GClasses {

using std::vector;

GVec::GVec(size_t n)
: m_size(n)
{
	if(n == 0)
		m_data = NULL;
	else
		m_data = new double[n];
}

GVec::GVec(int n)
: m_size(n)
{
	if(n == 0)
		m_data = NULL;
	else
		m_data = new double[n];
}

GVec::GVec(std::initializer_list<double> list)
: m_size(list.size())
{
	if(list.size() == 0)
		m_data = nullptr;
	else
		m_data = new double[list.size()];
	size_t i = 0;
	for(const double* it = begin(list); it != end(list); ++it)
	{
		m_data[i++] = *it;
	}
}

GVec::GVec(double d)
{
	throw Ex("Calling this method is an error");
}

GVec::GVec(GDomNode* pNode)
: m_data(NULL), m_size(0)
{
	deserialize(pNode);
}

GVec::GVec(const GVec& orig)
{
	m_size = orig.m_size;
	if(m_size == 0)
		m_data = NULL;
	else
	{
		m_data = new double[m_size];
		for(size_t i = 0; i < m_size; i++)
			m_data[i] = orig.m_data[i];
	}
}

GVec::~GVec()
{
	delete[] m_data;
}

GVec& GVec::operator=(const GVec& orig)
{
	resize(orig.m_size);
	for(size_t i = 0; i < m_size; i++)
		m_data[i] = orig.m_data[i];
	return *this;
}

void GVec::resize(size_t n)
{
	if(m_size == n)
		return;
	delete[] m_data;
	m_size = n;
	if(n == 0)
		m_data = NULL;
	else
		m_data = new double[n];
}

void GVec::resizePreserve(size_t n)
{
	if(m_size == n)
		return;
	size_t overlap = std::min(m_size, n);
	double* oldData = m_data;
	m_size = n;
	if(n == 0)
		m_data = NULL;
	else
		m_data = new double[n];
	for(size_t i = 0; i < overlap; i++)
		m_data[i] = oldData[i];
	delete[] oldData;
}

void GVec::fill(const double val, size_t startPos, size_t endPos)
{
	endPos = std::min(endPos, m_size);
	for(size_t i = startPos; i < endPos; i++)
		m_data[i] = val;
}

GVec GVec::operator+(const GVec& that) const
{
	GAssert(size() == that.size());
	GVec v(m_size);
	for(size_t i = 0; i < m_size; i++)
		v[i] = (*this)[i] + that[i];
	return v;
}

GVec& GVec::operator+=(const GVec& that)
{
	GAssert(size() == that.size());
	for(size_t i = 0; i < m_size; i++)
		(*this)[i] += that[i];
	return *this;
}

GVec& GVec::operator+=(const double scalar)
{
	for(size_t i = 0; i < m_size; i++)
		(*this)[i] += scalar;
	return *this;
}

GVec GVec::operator-(const GVec& that) const
{
	GAssert(size() == that.size());
	GVec v(m_size);
	for(size_t i = 0; i < m_size; i++)
		v[i] = (*this)[i] - that[i];
	return v;
}

GVec& GVec::operator-=(const GVec& that)
{
	GAssert(size() == that.size());
	for(size_t i = 0; i < m_size; i++)
		(*this)[i] -= that[i];
	return *this;
}

GVec GVec::operator*(double scalar) const
{
	GVec v(m_size);
	for(size_t i = 0; i < m_size; i++)
		v[i] = (*this)[i] * scalar;
	return v;
}

GVec& GVec::operator*=(double scalar)
{
	for(size_t i = 0; i < m_size; i++)
		(*this)[i] *= scalar;
	return *this;
}

GVec& GVec::operator*=(const GVec& that)
{
	GAssert(size() == that.size());
	for(size_t i = 0; i < m_size; i++)
		(*this)[i] *= that[i];
	return *this;
}

GVec& GVec::operator/=(double scalar)
{
	for(size_t i = 0; i < m_size; i++)
		(*this)[i] /= scalar;
	return *this;
}

void GVec::copy(const GVec& orig, size_t start, size_t size)
{
	GAssert(start <= orig.size());
	size = std::min(size, orig.size() - start);
	resize(size);
	for(size_t i = 0; i < size; i++)
		m_data[i] = orig[start + i];
}

void GVec::copy(const double* pSource, size_t n)
{
	resize(n);
	for(size_t i = 0; i < n; i++)
		(*this)[i] = *(pSource++);
}

void GVec::copy(size_t pos, const GVec& that, size_t start, size_t length)
{
	if(length == (size_t)-1)
		length = that.size() - start;
	else if(start + length > that.size())
		throw Ex("Input out of range. that size=", GClasses::to_str(that.size()), ", start=", GClasses::to_str(start), ", length=", GClasses::to_str(length));
	if(pos + length > m_size || start + length > that.m_size)
		throw Ex("Out of range. this size=", GClasses::to_str(m_size), ", pos=", GClasses::to_str(pos), ", that size=", GClasses::to_str(that.m_size));
	for(size_t i = 0; i < length; i++)
		(*this)[pos + i] = that[start + i];
}

double GVec::mean() const
{
	double m = 0.0;
	for(size_t i = 0; i < m_size; i++)
	{
		m += (*this)[i];
	}
	if ( m_size > 0 ) // make sure there are elements.
		return m / m_size;
	else
		return 0.0;
}

double GVec::squaredMagnitude() const
{
	double s = 0.0;
	for(size_t i = 0; i < m_size; i++)
	{
		double d = (*this)[i];
		s += (d * d);
	}
	return s;
}

void GVec::normalize()
{
	double mag = std::sqrt(squaredMagnitude());
	if(mag < 1e-16)
		fill(std::sqrt(1.0 / m_size));
	else
		(*this) *= (1.0 / mag);
}

void GVec::sumToOne()
{
	double s = sum();
	if(s < 1e-16)
		fill(1.0 / m_size);
	else
		(*this) *= (1.0 / s);
}

double GVec::squaredDistance(const GVec& that) const
{
	GAssert(size() == that.size());
	double s = 0.0;
	for(size_t i = 0; i < m_size; i++)
	{
		double d = (*this)[i] - that[i];
		s += (d * d);
	}
	return s;
}

void GVec::fillUniform(GRand& rand, double min, double max)
{
	for(size_t i = 0; i < m_size; i++)
		(*this)[i] = rand.uniform() * (max - min) + min;
}

void GVec::fillNormal(GRand& rand, double deviation)
{
	for(size_t i = 0; i < m_size; i++)
		(*this)[i] = rand.normal() * deviation;
}

void GVec::perturbNormal(GRand& rand, double deviation)
{
	for(size_t i = 0; i < m_size; i++)
		(*this)[i] += rand.normal() * deviation;
}

void GVec::fillSphericalShell(GRand& rand, double radius)
{
	fillNormal(rand);
	normalize();
	if(radius != 1.0)
		(*this) *= radius;
}

void GVec::fillSphericalVolume(GRand& rand)
{
	fillSphericalShell(rand);
	(*this) *= std::pow(rand.uniform(), 1.0 / m_size);
}

void GVec::fillSimplex(GRand& rand)
{
	for(size_t i = 0; i < m_size; i++)
		(*this)[i] = rand.exponential();
	(*this) *= (1.0 / sum());
}

void GVec::print(std::ostream& stream, char separator) const
{
	std::streamsize oldPrecision = stream.precision(14);
	if(m_size > 0)
		stream << (*this)[0];
	for(size_t i = 1; i < m_size; i++)
		stream << separator << (*this)[i];
	stream.precision(oldPrecision);
}

std::string GVec::to_str(char separator) const
{
	std::ostringstream ss;
	print(ss, separator);
	return ss.str();
}

double GVec::sum() const
{
	double s = 0.0;
	for(size_t i = 0; i < m_size; i++)
		s += (*this)[i];
	return s;
}

size_t GVec::indexOfMax(size_t startPos, size_t endPos) const
{
	endPos = std::min(m_size, endPos);
	size_t maxIndex = startPos;
	double maxValue = -1e300;
	for(size_t i = startPos; i < endPos; i++)
	{
		if((*this)[i] > maxValue)
		{
			maxIndex = i;
			maxValue = (*this)[i];
		}
	}
	return maxIndex;
}

size_t GVec::indexOfMin(size_t startPos, size_t endPos) const
{
	endPos = std::min(m_size, endPos);
	size_t minIndex = startPos;
	double minValue = 1e300;
	for(size_t i = startPos; i < endPos; i++)
	{
		if((*this)[i] < minValue)
		{
			minIndex = i;
			minValue = (*this)[i];
		}
	}
	return minIndex;
}

double GVec::max(size_t startPos, size_t endPos) const
{
	endPos = std::min(m_size, endPos);
	double maxValue = -1e300;
	for(size_t i = startPos; i < endPos; i++)
	{
		if((*this)[i] > maxValue)
		{
			maxValue = (*this)[i];
		}
	}
	return maxValue;
}

double GVec::min(size_t startPos, size_t endPos) const
{
	endPos = std::min(m_size, endPos);
	double minValue = 1e300;
	for(size_t i = startPos; i < endPos; i++)
	{
		if((*this)[i] < minValue)
		{
			minValue = (*this)[i];
		}
	}
	return minValue;
}

GDomNode* GVec::serialize(GDom* pDoc) const
{
	GDomNode* pNode = pDoc->newList();
	for(size_t i = 0; i < m_size; i++)
		pNode->add(pDoc, (*this)[i]);
	return pNode;
}

void GVec::deserialize(const GDomNode* pNode)
{
	GDomListIterator it(pNode);
	resize(it.remaining());
	for(size_t i = 0; it.current(); i++)
	{
		(*this)[i] = it.currentDouble();
		it.advance();
	}
}

double GVec::dotProduct(const GVec& that) const
{
	GAssert(size() == that.size());
	double s = 0.0;
	for(size_t i = 0; i < m_size; i++)
		s += ((*this)[i] * that[i]);
	return s;
}

double GVec::dotProductIgnoringUnknowns(const GVec& that) const
{
	GAssert(size() == that.size());
	double s = 0.0;
	for(size_t i = 0; i < m_size; i++)
	{
		if((*this)[i] != UNKNOWN_REAL_VALUE && that[i] != UNKNOWN_REAL_VALUE)
			s += ((*this)[i] * that[i]);
	}
	return s;
}

double GVec::dotProductIgnoringUnknowns(const GVec& from, const GVec& to) const
{
	GAssert(size() == from.size());
	GAssert(size() == to.size());
	double s = 0.0;
	for(size_t i = 0; i < m_size; i++)
	{
		if((*this)[i] != UNKNOWN_REAL_VALUE && from[i] != UNKNOWN_REAL_VALUE && to[i] != UNKNOWN_REAL_VALUE)
			s += ((*this)[i] * (to[i] - from[i]));
	}
	return s;
}

double GVec::estimateSquaredDistanceWithUnknowns(const GVec& that) const
{
	GAssert(size() == that.size());
	double dist = 0;
	double d;
	size_t nMissing = 0;
	for(size_t n = 0; n < m_size; n++)
	{
		if((*this)[n] == UNKNOWN_REAL_VALUE || that[n] == UNKNOWN_REAL_VALUE)
			nMissing++;
		else
		{
			d = (*this)[n] - that[n];
			dist += (d * d);
		}
	}
	if(nMissing >= m_size)
		return 1e50; // we have no info, so let's make a wild guess
	else
		return dist * m_size / (m_size - nMissing);
}

void GVec::addScaled(double scalar, const GVec& that, size_t start, size_t size)
{
	GAssert(start + size <= that.size() || size == (size_t)-1);
	size = std::min(size, that.size() - start);
	GAssert(this->size() == size);
	for(size_t i = 0; i < size; i++)
		(*this)[i] += (scalar * that[start + i]);
}

void GVec::regularizeL1(double amount)
{
	for(size_t i = 0; i < m_size; i++)
	{
		if((*this)[i] < 0.0)
			(*this)[i] = std::min(0.0, (*this)[i] + amount);
		else
			(*this)[i] = std::max(0.0, (*this)[i] - amount);
	}
}

void GVec::erase(size_t start, size_t count)
{
	if(start + count > m_size)
		throw Ex("out of range");
	size_t end = m_size - count;
	for(size_t i = start; i < end; i++)
		(*this)[i] = (*this)[i + count];
	m_size -= count;
}

double GVec::correlation(const GVec& that) const
{
	double d = this->dotProduct(that);
	if(d == 0.0)
		return 0.0;
	return d / (sqrt(this->squaredMagnitude() * that.squaredMagnitude()));
}

void GVec::clip(double min, double max)
{
	GAssert(max >= min);
	for(size_t i = 0; i < m_size; i++)
		(*this)[i] = std::max(min, std::min(max, (*this)[i]));
}

void GVec::subtractComponent(const GVec& component)
{
	GAssert(size() == component.size());
	double comp = dotProduct(component);
	for(size_t i = 0; i < m_size; i++)
		(*this)[i] -= component[i] * comp;
}

void GVec::toImage(GImage* pImage, int width, int height, int channels, double range) const
{
	if(size() != (size_t)width * (size_t)height * (size_t)channels)
		throw Ex("Size mismatch");
	pImage->setSize(width, height);
	unsigned int* pix = pImage->pixels();
	if(channels == 3)
	{
		size_t pos = 0;
		for(int y = 0; y < height; y++)
		{
			for(int x = 0; x < width; x++)
			{
				int r = ClipChan((int)((*this)[pos++] * 256 / range));
				int g = ClipChan((int)((*this)[pos++] * 256 / range));
				int b = ClipChan((int)((*this)[pos++] * 256 / range));
				*(pix++) = gARGB(0xff, r, g, b);
			}
		}
	}
	else if(channels == 1)
	{
		size_t pos = 0;
		for(int y = 0; y < height; y++)
		{
			for(int x = 0; x < width; x++)
			{
				int v = ClipChan((int)((*this)[pos++] * 256 / range));
				*(pix++) = gARGB(0xff, v, v, v);
			}
		}
	}
	else
		throw Ex("unsupported value for channels");
}

void GVec::fromImage(GImage* pImage, int width, int height, int channels, double range)
{
	resize(width * height * channels);
	unsigned int* pix = pImage->pixels();
	if(channels == 3)
	{
		size_t pos = 0;
		for(int y = 0; y < height; y++)
		{
			for(int x = 0; x < width; x++)
			{
				(*this)[pos++] = gRed(*pix) * range / 255;
				(*this)[pos++] = gGreen(*pix) * range / 255;
				(*this)[pos++] = gBlue(*pix) * range / 255;
				pix++;
			}
		}
	}
	else if(channels == 1)
	{
		size_t pos = 0;
		for(int y = 0; y < height; y++)
		{
			for(int x = 0; x < width; x++)
			{
				(*this)[pos++] = gGray(*pix) * range / MAX_GRAY_VALUE;
				pix++;
			}
		}
	}
	else
		throw Ex("unsupported value for channels");
}

void GVec::swapContents(GVec& that)
{
	std::swap(m_data, that.m_data);
	std::swap(m_size, that.m_size);
}






















// static
void GVec::setAll(double* pVector, double value, size_t dims)
{
	for(size_t i = 0; i < dims; i++)
	{
		*pVector = value;
		pVector++;
	}
}

void GVec::fill(double* pVector, double value, size_t dims)
{
	for(size_t i = 0; i < dims; i++)
	{
		*pVector = value;
		pVector++;
	}
}

// static
void GVec::perturb(double* pDest, double deviation, size_t dims, GRand& rand)
{
	for(size_t i = 0; i < dims; i++)
		*(pDest++) += deviation * rand.normal();
}

#ifndef MIN_PREDICT
// static
void GVec::test()
{
	{
		// Test some static methods
		GRand prng(0);
		GVec v1(100);
		GVec v2(100);
		for(int i = 0; i < 10; i++)
		{
			v1.fillSphericalShell(prng);
			v2.fillSphericalShell(prng);
			v2.subtractComponent(v1);
			v2.normalize();
			if(std::abs(v1.correlation(v2)) > 1e-4)
				throw Ex("Failed");
			if(std::abs(v1.squaredMagnitude() - 1) > 1e-4)
				throw Ex("Failed");
			if(std::abs(v2.squaredMagnitude() - 1) > 1e-4)
				throw Ex("Failed");
		}
	}

	// Test the basic operations of the GVec object
	GVec v1(2);
	v1[0] = 2.0;
	v1[1] = 7.0;
	GVec v2(v1);
	if(v2.size() != 2)
		throw Ex("failed");
	if(v1.squaredDistance(v2) != 0.0)
		throw Ex("failed");
	std::swap(v1[0], v1[1]);
	if(v1.squaredDistance(v2) != 50.0)
		throw Ex("failed");
	v2.fill(3.0);
	v1 = v2;
	if(v1.squaredMagnitude() != 18.0)
		throw Ex("failed");
	if(v1.data()[0] != 3.0 || v1.data()[1] != 3.0)
		throw Ex("failed");

	// Test overloaded operators
	v1[0] = 1.0;
	v1[1] = 2.0;
	v2[0] = 3.0;
	v2[1] = 4.0;
	GVec v3 = v1 + v2;
	if(v3.squaredMagnitude() != 52.0)
		throw Ex("failed");
	v3 += v1;
	if(v3.squaredMagnitude() != 89.0)
		throw Ex("failed");
	v1 *= 2.0;
	if(v1.squaredMagnitude() != 20.0)
		throw Ex("failed");
	v1 -= v2;
	if(v1[0] != -1.0)
		throw Ex("failed");
	if(v1[1] != 0.0)
		throw Ex("failed");

	// Test some other methods:
	GVec v4(10);
	GVec v5(10);
	for(size_t i = 0; i < 10; i++ )
	{
		v4[i] = (double)i;
		v5[i] = (double)(20 - i);
	}
	double max = v4.max();
	if ( max != 9 )
		throw Ex("failed");
	max = v5.max();
	if ( max != 20 )
		throw Ex("failed");
	v4 += 2;
	if ( v4[0] != 2 || v4[1] != 3 || v4[2] != 4 )
		throw Ex("failed");
	double mean = v4.mean();
	if ( mean != 6.5 )
		throw Ex("failed");
	GVec v6(2);
	v6[0] = 10;
	v6[1] = 10;
	v6 /= 2;
	if ( v6[0] != 5 || v6[1] != 5 )
		throw Ex("failed");
}
#endif // MIN_PREDICT

std::string to_str(const GVec& v)
{
	return v.to_str();
}






GIndexVec::GIndexVec(size_t n)
: m_size(n)
{
	if(n == 0)
		m_data = nullptr;
	else
		m_data = new size_t[n];
}

GIndexVec::GIndexVec(std::initializer_list<size_t> list)
: m_size(list.size())
{
	if(list.size() == 0)
		m_data = nullptr;
	else
		m_data = new size_t[list.size()];
	size_t i = 0;
	for(const size_t* it = begin(list); it != end(list); ++it)
	{
		m_data[i++] = *it;
	}
}

GIndexVec::GIndexVec(const GIndexVec& copyMe)
{
	if(copyMe.size() == 0)
		m_data = nullptr;
	else
		m_data = new size_t[copyMe.m_size];
	m_size = copyMe.m_size;
	memcpy(m_data, copyMe.m_data, sizeof(size_t) * m_size);
}

GIndexVec::GIndexVec(GDomNode* pNode)
: m_data(nullptr), m_size(0)
{
	GDomListIterator it(pNode);
	resize(it.remaining());
	for(size_t i = 0; it.current(); i++)
	{
		(*this)[i] = it.currentInt();
		it.advance();
	}	
}

GIndexVec::~GIndexVec()
{
	delete[] m_data;
}

GDomNode* GIndexVec::serialize(GDom* pDoc) const
{
	GDomNode* pNode = pDoc->newList();
	for(size_t i = 0; i < m_size; i++)
		pNode->add(pDoc, (*this)[i]);
	return pNode;
}

void GIndexVec::resize(size_t n)
{
	delete[] m_data;
	m_size = n;
	if(n == 0)
		m_data = NULL;
	else
		m_data = new size_t[n];
}

void GIndexVec::fillIndexes()
{
	for(size_t i = 0; i < m_size; i++)
		m_data[i] = i;
}

size_t GIndexVec::popRandom(GRand& rand)
{
	if(m_size == 0)
		throw Ex("Nothing to pop");
	size_t r = rand.next(m_size);
	size_t v = m_data[r];
	m_data[r] = m_data[m_size - 1];
	m_size--;
	return v;
}




// static
void GIndexVec::makeIndexVec(size_t* pVec, size_t size)
{
	for(size_t i = 0; i < size; i++)
	{
		*pVec = i;
		pVec++;
	}
}

// static
void GIndexVec::shuffle(size_t* pVec, size_t size, GRand* pRand)
{
	for(size_t i = size; i > 1; i--)
	{
		size_t r = (size_t)pRand->next(i);
		size_t t = pVec[i - 1];
		pVec[i - 1] = pVec[r];
		pVec[r] = t;
	}
}

// static
void GIndexVec::setAll(size_t* pVec, size_t value, size_t size)
{
	while(size > 0)
	{
		*pVec = value;
		pVec++;
		size--;
	}
}

// static
void GIndexVec::copy(size_t* pDest, const size_t* pSource, size_t nDims)
{
	memcpy(pDest, pSource, sizeof(size_t) * nDims);
}

// static
size_t GIndexVec::maxValue(size_t* pVec, size_t size)
{
	size_t m = *(pVec++);
	size--;
	while(size > 0)
	{
		m = std::max(m, *(pVec++));
		size--;
	}
	return m;
}

// static
size_t GIndexVec::indexOfMax(size_t* pVec, size_t size)
{
	size_t index = 0;
	size_t m = *(pVec++);
	size--;
	size_t i = 1;
	while(size > 0)
	{
		if(*pVec > m)
		{
			m = *pVec;
			index = i;
		}
		pVec++;
		size--;
		i++;
	}
	return index;
}

// static
GDomNode* GIndexVec::serialize(GDom* pDoc, const size_t* pVec, size_t dims)
{
	GDomNode* pNode = pDoc->newList();
	for(size_t i = 0; i < dims; i++)
		pNode->add(pDoc, *(pVec++));
	return pNode;
}

// static
void GIndexVec::deserialize(size_t* pVec, GDomListIterator& it)
{
	while(it.current())
	{
		*(pVec++) = size_t(it.currentInt());
		it.advance();
	}
}

// static
void GIndexVec::print(std::ostream& stream, size_t* pVec, size_t dims)
{
	if(dims == 0)
		return;
	stream << *pVec;
	pVec++;
	for(size_t i = 1; i < dims; i++)
	{
		stream << ", ";
		stream << *pVec;
		pVec++;
	}
}




GRandomIndexIterator::GRandomIndexIterator(size_t len, GRand& rnd)
: m_length(len), m_rand(rnd)
{
	m_pIndexes = new size_t[len];
	size_t* pInd = m_pIndexes;
	for(size_t i = 0; i < len; i++)
		*(pInd++) = i;
	m_pEnd = m_pIndexes + len;
	m_pCur = m_pEnd;
}

GRandomIndexIterator::~GRandomIndexIterator()
{
	delete[] m_pIndexes;
}

void GRandomIndexIterator::reset()
{
	for(size_t i = m_length; i > 1; i--)
		std::swap(m_pIndexes[i - 1], m_pIndexes[m_rand.next(i)]);
	m_pCur = m_pIndexes;
}

void GRandomIndexIterator::resetPart(size_t size)
{
	for(size_t i = size; i > 1; i--)
		std::swap(m_pIndexes[i - 1], m_pIndexes[m_rand.next(i)]);
	m_pCur = m_pIndexes;
}

bool GRandomIndexIterator::next(size_t& outIndex)
{
	if(m_pCur == m_pEnd)
		return false;
	outIndex = *(m_pCur++);
	return true;
}





GCoordVectorIterator::GCoordVectorIterator(size_t dimCount, size_t* pRanges)
{
	m_pCoords = NULL;
	reset(dimCount, pRanges);
}

GCoordVectorIterator::GCoordVectorIterator(vector<size_t>& range)
{
	m_pCoords = NULL;
	reset(range);
}

GCoordVectorIterator::~GCoordVectorIterator()
{
	delete[] m_pCoords;
}

void GCoordVectorIterator::reset()
{
	memset(m_pCoords, '\0', sizeof(size_t) * m_dims);
	m_sampleShift = INVALID_INDEX;
}

void GCoordVectorIterator::reset(size_t dimCount, size_t* pRanges)
{
	m_dims = dimCount;
	delete[] m_pCoords;
	if(dimCount > 0)
	{
		m_pCoords = new size_t[2 * dimCount];
		m_pRanges = m_pCoords + dimCount;
		if(pRanges)
			memcpy(m_pRanges, pRanges, sizeof(size_t) * dimCount);
		else
		{
			for(size_t i = 0; i < dimCount; i++)
				m_pRanges[i] = 1;
		}
	}
	else
	{
		m_pCoords = NULL;
		m_pRanges = NULL;
	}
	reset();
}

void GCoordVectorIterator::reset(vector<size_t>& range)
{
	m_dims = range.size();
	delete[] m_pCoords;
	if(m_dims > 0)
	{
		m_pCoords = new size_t[2 * m_dims];
		m_pRanges = m_pCoords + m_dims;
		for(size_t i = 0; i < m_dims; i++)
			m_pRanges[i] = range[i];
	}
	else
	{
		m_pCoords = NULL;
		m_pRanges = NULL;
	}
	reset();
}

size_t GCoordVectorIterator::coordCount()
{
	size_t n = 1;
	size_t* pR = m_pRanges;
	for(size_t i = 0; i < m_dims; i++)
		n *= (*(pR++));
	return n;
}

bool GCoordVectorIterator::advance()
{
	size_t j;
	for(j = 0; j < m_dims; j++)
	{
		if(++m_pCoords[j] >= m_pRanges[j])
			m_pCoords[j] = 0;
		else
			break;
	}

	// Test if we're done
	if(j >= m_dims)
		return false;
	return true;
}

bool GCoordVectorIterator::advance(size_t steps)
{
	size_t j;
	for(j = 0; j < m_dims; j++)
	{
		size_t t = m_pCoords[j] + steps;
		m_pCoords[j] = t % m_pRanges[j];
		steps = t / m_pRanges[j];
		if(t == 0)
			break;
	}

	// Test if we're done
	if(j >= m_dims)
		return false;
	return true;
}

#ifndef MIN_PREDICT
bool GCoordVectorIterator::advanceSampling()
{
	if(m_sampleShift == INVALID_INDEX) // if we have not yet computed the step size
	{
		size_t r = m_pRanges[0];
		for(size_t i = 1; i < m_dims; i++)
			r = std::max(r, m_pRanges[i]);
		m_sampleShift = GBits::boundingShift(r);
		m_sampleMask = 0;
	}

	m_pCoords[0] += ((size_t)1 << (m_sampleShift + (m_sampleMask ? 0 : 1)));
	if(m_pCoords[0] >= m_pRanges[0])
	{
		m_pCoords[0] = 0;
		size_t j = 1;
		for( ; j < m_dims; j++)
		{
			m_pCoords[j] += ((size_t)1 << m_sampleShift);
			m_sampleMask ^= ((size_t)1 << j);
			if(m_pCoords[j] < m_pRanges[j])
				break;
			m_pCoords[j] = 0;
			m_sampleMask &= ~((size_t)1 << j);
		}
		if(j >= m_dims)
		{
			if(--m_sampleShift == INVALID_INDEX) // if we're all done
				return false;
		}
		if(m_sampleMask == 0)
		{
			m_pCoords[0] -= ((size_t)1 << m_sampleShift);
			return advanceSampling();
		}
	}
	return true;
}
#endif // MIN_PREDICT

size_t* GCoordVectorIterator::current()
{
	return m_pCoords;
}

void GCoordVectorIterator::currentNormalized(double* pCoords)
{
	for(size_t i = 0; i < m_dims; i++)
	{
		*pCoords = ((double)m_pCoords[i] + 0.5) / m_pRanges[i];
		pCoords++;
	}
}

size_t GCoordVectorIterator::currentIndex()
{
	size_t index = 0;
	size_t n = 1;
	for(size_t i = 0; i < m_dims; i++)
	{
		index += n * m_pCoords[i];
		n *= m_pRanges[i];
	}
	return index;
}

void GCoordVectorIterator::setRandom(GRand* pRand)
{
	for(size_t i = 0; i < m_dims; i++)
		m_pCoords[i] = (size_t)pRand->next(m_pRanges[i]);
}

#ifndef MIN_PREDICT
#define TEST_DIMS 4
// static
void GCoordVectorIterator::test()
{
	size_t r = 11;
	size_t size = 1;
	for(size_t i = 0; i < TEST_DIMS; i++)
		size *= r;
	GBitTable bt(size);
	size_t ranges[TEST_DIMS];
	for(size_t i = 0; i < TEST_DIMS; i++)
		ranges[i] = r;
	GCoordVectorIterator cvi(TEST_DIMS, ranges);
	size_t count = 0;
	while(true)
	{
		size_t index = cvi.currentIndex();
		if(bt.bit(index))
			throw Ex("already got this one");
		bt.set(index);
		count++;
		if(!cvi.advanceSampling())
			break;
	}
	if(count != size)
		throw Ex("didn't get them all");
}
#endif // MIN_PREDICT






size_t countTensorSize(const GIndexVec& dims)
{
	size_t n = 1;
	for(size_t i = 0; i < dims.size(); i++)
		n *= dims[i];
	return n;
}

GTensor::GTensor(GVec& vals)
: GVecWrapper(vals), dims(1)
{
	dims[0] = vals.size();
}

GTensor::GTensor(GVec& vals, size_t width, size_t height)
: GVecWrapper(vals), dims(2)
{
	if(width * height != vals.size())
		throw Ex("Mismatching sizes");
	dims[0] = width;
	dims[1] = height;
}

GTensor::GTensor(GVec& vals, size_t a, size_t b, size_t c)
: GVecWrapper(vals), dims(3)
{
	if(a * b * c != vals.size())
		throw Ex("Mismatching sizes");
	dims[0] = a;
	dims[1] = b;
	dims[2] = c;
}

GTensor::GTensor(GVec& vals, size_t a, size_t b, size_t c, size_t d)
: GVecWrapper(vals), dims(4)
{
	if(a * b * c * d != vals.size())
		throw Ex("Mismatching sizes");
	dims[0] = a;
	dims[1] = b;
	dims[2] = c;
	dims[3] = d;
}

GTensor::GTensor(const GTensor& copyMe)
: GVecWrapper(copyMe.m_data, copyMe.m_size),
dims(copyMe.dims)
{
}

GTensor::GTensor(double* buf, const GIndexVec& _dims)
: GVecWrapper(buf, buf ? countTensorSize(_dims) : 0),
dims(_dims)
{
}

GTensor::GTensor(GDomNode* pNode)
: GVecWrapper(nullptr, 0),
dims(pNode)
{
}

void GTensor::convolve(const GTensor& in, const GTensor& filter, GTensor& out, bool flipFilter, size_t stride)
{
	// Precompute some values
	size_t dc = in.dims.size();
	GAssert(dc == filter.dims.size());
	GAssert(dc == out.dims.size());
	size_t* kinner = (size_t*)alloca(sizeof(size_t) * 5 * dc);
	size_t* kouter = kinner + dc;
	size_t* stepInner = kouter + dc;
	size_t* stepFilter = stepInner + dc;
	size_t* stepOuter = stepFilter + dc;

	// Compute step sizes
	stepInner[0] = 1;
	stepFilter[0] = 1;
	stepOuter[0] = 1;
	for(size_t i = 1; i < dc; i++)
	{
		stepInner[i] = stepInner[i - 1] * in.dims[i - 1];
		stepFilter[i] = stepFilter[i - 1] * filter.dims[i - 1];
		stepOuter[i] = stepOuter[i - 1] * out.dims[i - 1];
	}
	size_t filterTail = stepFilter[dc - 1] * filter.dims[dc - 1] - 1;

	// Do convolution
	size_t op = 0;
	size_t ip = 0;
	size_t fp = 0;
	for(size_t i = 0; i < dc; i++)
	{
		kouter[i] = 0;
		kinner[i] = 0;
		ssize_t padding = (stride * (out.dims[i] - 1) + filter.dims[i] - in.dims[i]) / 2;
		ssize_t adj = (padding - std::min(padding, (ssize_t)kouter[i])) - kinner[i];
		kinner[i] += adj;
		fp += adj * stepFilter[i];
	}
	while(true) // kouter
	{
		double val = 0.0;

		// Fix up the initial kinner positions
		for(size_t i = 0; i < dc; i++)
		{
			ssize_t padding = (stride * (out.dims[i] - 1) + filter.dims[i] - in.dims[i]) / 2;
			ssize_t adj = (padding - std::min(padding, (ssize_t)kouter[i])) - kinner[i];
			kinner[i] += adj;
			fp += adj * stepFilter[i];
			ip += adj * stepInner[i];
		}
		while(true) // kinner
		{
			val += (in[ip] * filter[flipFilter ? filterTail - fp : fp]);

			// increment the kinner position
			size_t i;
			for(i = 0; i < dc; i++)
			{
				kinner[i]++;
				ip += stepInner[i];
				fp += stepFilter[i];
				ssize_t padding = (stride * (out.dims[i] - 1) + filter.dims[i] - in.dims[i]) / 2;
				if(kinner[i] < filter.dims[i] && kouter[i] + kinner[i] - padding < in.dims[i])
					break;
				ssize_t adj = (padding - std::min(padding, (ssize_t)kouter[i])) - kinner[i];
				kinner[i] += adj;
				fp += adj * stepFilter[i];
				ip += adj * stepInner[i];
			}
			if(i >= dc)
				break;
		}
		out[op] += val;

		// increment the kouter position
		size_t i;
		for(i = 0; i < dc; i++)
		{
			kouter[i]++;
			op += stepOuter[i];
			ip += stride * stepInner[i];
			if(kouter[i] < out.dims[i])
				break;
			op -= kouter[i] * stepOuter[i];
			ip -= kouter[i] * stride * stepInner[i];
			kouter[i] = 0;
		}
		if(i >= dc)
			break;
	}
}

// static
void GTensor::test()
{
	{
		// 1D test
		GVec in(5);
		in[0] = 2;
		in[1] = 3;
		in[2] = 1;
		in[3] = 0;
		in[4] = 1;
		GTensor tin(in);

		GVec k(3);
		k[0] = 1;
		k[1] = 0;
		k[2] = 2;
		GTensor tk(k);

		GVec out(7);
		GTensor tout(out);

		GTensor::convolve(tin, tk, tout, true, 1);

		//     2 3 1 0 1
		// 2 0 1 --->
		if(std::abs(2 - out[0]) > 1e-10) throw Ex("wrong");
		if(std::abs(3 - out[1]) > 1e-10) throw Ex("wrong");
		if(std::abs(5 - out[2]) > 1e-10) throw Ex("wrong");
		if(std::abs(6 - out[3]) > 1e-10) throw Ex("wrong");
		if(std::abs(3 - out[4]) > 1e-10) throw Ex("wrong");
		if(std::abs(0 - out[5]) > 1e-10) throw Ex("wrong");
		if(std::abs(2 - out[6]) > 1e-10) throw Ex("wrong");
	}

	{
		// 2D test
		GVec in(9);
		in[0] = 1; in[1] = 2; in[2] = 3;
		in[3] = 4; in[4] = 5; in[5] = 6;
		in[6] = 7; in[7] = 8; in[8] = 9;
		GTensor tin(in, 3, 3);

		GVec k(9);
		k[0] = 1; k[1] = 2; k[2] = 1;
		k[3] = 0; k[4] = 0; k[5] = 0;
		k[6] = -1; k[7] = -2; k[8] = -1;
		GTensor tk(k, 3, 3);

		GVec out(9);
		GTensor tout(out, 3, 3);

		GTensor::convolve(tin, tk, tout, false, 1);

		if(std::abs(-13 - out[0]) > 1e-10) throw Ex("wrong");
		if(std::abs(-20 - out[1]) > 1e-10) throw Ex("wrong");
		if(std::abs(-17 - out[2]) > 1e-10) throw Ex("wrong");
		if(std::abs(-18 - out[3]) > 1e-10) throw Ex("wrong");
		if(std::abs(-24 - out[4]) > 1e-10) throw Ex("wrong");
		if(std::abs(-18 - out[5]) > 1e-10) throw Ex("wrong");
		if(std::abs(13 - out[6]) > 1e-10) throw Ex("wrong");
		if(std::abs(20 - out[7]) > 1e-10) throw Ex("wrong");
		if(std::abs(17 - out[8]) > 1e-10) throw Ex("wrong");
	}
}



} // namespace GClasses
