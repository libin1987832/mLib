
#ifndef _BASEIMAGE_H_
#define _BASEIMAGE_H_

#include "baseImageHelper.h"

namespace ml {


	class Image {
	public:

        enum Format {
            FORMAT_ColorImageR8G8B8A8,
            FORMAT_ColorImageR32G32B32A32,
            FORMAT_ColorImageR32G32B32,
            FORMAT_DepthImage,
            FORMAT_DepthImage16,
            FORMAT_Unknown
        };

		Image(Format format = FORMAT_Unknown) {
			m_format = format;
		}

		virtual ~Image() {}

		Format getFormat() const {
			return m_format;
		}

		void setFormat(Format format) {
			m_format = format;
		}
	protected:
		

		Format m_format;
	};

    namespace BaseImageHelper {
        template<class T> inline Image::Format formatFromTemplate() {
            return Image::FORMAT_Unknown;
        }
        template<> inline Image::Format formatFromTemplate<vec4uc>() {
            return Image::FORMAT_ColorImageR8G8B8A8;
        }
        template<> inline Image::Format formatFromTemplate<vec4f>() {
            return Image::FORMAT_ColorImageR32G32B32A32;
        }
        template<> inline Image::Format formatFromTemplate<vec3f>() {
            return Image::FORMAT_ColorImageR32G32B32;
        }
        template<> inline Image::Format formatFromTemplate<float>() {
            return Image::FORMAT_DepthImage;
        }
        template<> inline Image::Format formatFromTemplate<unsigned short>() {
            return Image::FORMAT_DepthImage16;
        }
    };

	template <class T>
	class BaseImage : public Image {
	public:

		// TODO: use templating to collapse iterator and const-iterator
        struct iteratorEntry
        {
            iteratorEntry(size_t _x, size_t _y, T &_value)
                : x(_x), y(_y), value(_value)
            {

            }
            size_t x;
            size_t y;
            T &value;
        };

        struct constIteratorEntry
        {
            constIteratorEntry(size_t _x, size_t _y, const T &_value)
                : x(_x), y(_y), value(_value)
            {

            }
            size_t x;
            size_t y;
            const T &value;
        };

        struct iterator
        {
            iterator(BaseImage<T> *_image)
            {
                x = 0;
                y = 0;
                image = _image;
            }
            iterator(const iterator &i)
            {
                x = i.x;
                y = i.x;
                image = i.image;
            }
            ~iterator() {}
            iterator& operator=(const iterator &i)
            {
                x = i.x;
                y = i.y;
                image = i.image;
                return *this;
            }
            iterator& operator++()
            {
                x++;
                if (x == image->getWidth())
                {
                    x = 0;
                    y++;
                    if (y == image->getHeight())
                    {
                        image = NULL;
                    }
                }
                return *this;
            }
            iteratorEntry operator* () const
            {
                return iteratorEntry(x, y, (*image)(x, y));
            }

            bool operator != (const iterator &i) const
            {
                return i.image != image;
            }

            friend void swap(iterator &a, iterator &b);

            size_t x, y;

        private:
            BaseImage<T> *image;
        };

        struct constIterator
        {
            constIterator(const BaseImage<T> *_image)
            {
                x = 0;
                y = 0;
                image = _image;
            }
            constIterator(const iterator &i)
            {
                x = i.x;
                y = i.x;
                image = i.image;
            }
            ~constIterator() {}
            constIterator& operator=(const iterator &i)
            {
                x = i.x;
                y = i.y;
                image = i.image;
                return *this;
            }
            constIterator& operator++()
            {
                x++;
                if (x == image->getWidth())
                {
                    x = 0;
                    y++;
                    if (y == image->getHeight())
                    {
                        image = NULL;
                    }
                }
                return *this;
            }
            constIteratorEntry operator* () const
            {
                return constIteratorEntry(x, y, (*image)(x, y));
            }

            bool operator != (const constIterator &i) const
            {
                return i.image != image;
            }

            friend void swap(constIterator &a, constIterator &b);

            size_t x, y;

        private:
            const BaseImage<T> *image;
        };

		BaseImage() : Image(BaseImageHelper::formatFromTemplate<T>()) {
			m_data = nullptr;
			m_width = m_height = 0;
		}

        BaseImage(const vec2i &dimensions) {
            create((UINT)dimensions.x, (UINT)dimensions.y);
        }
        
        BaseImage(size_t width, size_t height, const T *data = nullptr) : Image(BaseImageHelper::formatFromTemplate<T>()) {
            //
            // TODO: size_t is probably the more appropriate type here
            //
			create((UINT)width, (UINT)height);

			if (data) {
				memcpy(m_data, data, sizeof(T) * height * width);
			}
		}

		//! Copy constructor
		BaseImage(const BaseImage& other) : Image(other.getFormat()){
			create(other.m_width, other.m_height);
			memcpy(m_data, other.m_data, sizeof(T) * m_width * m_height);
			m_InvalidValue = other.getInvalidValue();
		}

		//! Move constructor
        BaseImage(BaseImage&& other) : Image(BaseImageHelper::formatFromTemplate<T>()) {
			m_data = nullptr;
			m_width = m_height = 0;
			swap(*this, other);
		}

		//! Copy constructor for other classes
		template<class U>
        BaseImage(const BaseImage<U>& other) : Image(BaseImageHelper::formatFromTemplate<T>()) {
			create(other.m_width, other.m_height);
			for (unsigned int i = 0; i < m_height*m_width; i++) {
				BaseImageHelper::convertBaseImagePixel<T, U>(m_data[i], other.getDataPointer()[i]);
			}
			const U& otherInvalidValue = other.getInvalidValue();
			BaseImageHelper::convertBaseImagePixel<T, U>(m_InvalidValue, otherInvalidValue);
		}

		//! clears the image; and release all memory
		void free() {
			SAFE_DELETE_ARRAY(m_data);
			m_height = m_width = 0;
		}


		//! adl swap
		friend void swap(BaseImage& a, BaseImage& b) {
			std::swap(a.m_data, b.m_data);
			std::swap(a.m_height, b.m_height);
			std::swap(a.m_width, b.m_width);
			std::swap(a.m_InvalidValue, b.m_InvalidValue);
			std::swap(a.m_format, b.m_format);
		}

		void initialize(const T *data = nullptr)
		{
			if (data) {
				memcpy(m_data, data, sizeof(T) * m_height * m_width);
			}
		}

		~BaseImage(void) {
			SAFE_DELETE_ARRAY(m_data);
		}

        iterator begin()
        {
            return iterator(this);
        }

        iterator end()
        {
            return iterator(NULL);
        }

        constIterator begin() const
        {
            return constIterator(this);
        }

        constIterator end() const
        {
            return constIterator(NULL);
        }

		//! Returns the difference of two images (current - other)
		BaseImage<T> operator-(const BaseImage<T> &other) const {
			if (other.m_width != m_width || other.m_height != m_height)	throw MLIB_EXCEPTION("Invalid image dimensions");
			BaseImage<T> im(m_width, m_height);
			for (unsigned int i = 0; i < m_width * m_height; i++) {
				im.m_data[i] = m_data[i] - other.m_data[i];
			}
			return im;
		}
		//! Returns the sum of two images (current + other)
		BaseImage<T> operator+(const BaseImage<T> &other) const {
			if (other.m_width != m_width || other.m_height != m_height)	throw MLIB_EXCEPTION("Invalid image dimensions");
			BaseImage<T> im(m_width, m_height);
			for (unsigned int i = 0; i < m_width * m_height; i++) {
				im.m_data[i] = m_data[i] + other.m_data[i];
			}
			return im;
		}

		//! Mutator Operator (unsigned int)
		T& operator()(unsigned int x, unsigned int y) {
			MLIB_ASSERT(x < m_width && y < m_height);
			return m_data[y*m_width + x];
		}

		//! Mutator Operator (int)
		T& operator()(int x, int y) {
			MLIB_ASSERT((unsigned int)x < m_width && (unsigned int)y < m_height);
			return m_data[y*m_width + x];
		}

		//! Mutator Operator (double); x,y \in [0;1]
		T& operator()(double x, double y) {
			return (*this)((unsigned int)math::round(x*(m_width - 1)), (unsigned int)math::round(y*(m_height - 1)));
		}

		//! Mutator Operator (float); x,y \in [0;1]
		T& operator()(float x, float y) {
			return (*this)((unsigned int)math::round(x*(m_width - 1)), (unsigned int)math::round(y*(m_height - 1)));
		}

		template <class S>
		void setPixel(S x, S y, const T& value) {
			(*this)(x, y) = value;
		}

		//! Access Operator (unsigned int)
		const T& operator()(unsigned int x, unsigned int y) const {
			MLIB_ASSERT(x < m_width && y < m_height);
			return m_data[y*m_width + x];
		}

		//! Access Operator (size_t)
		const T& operator()(size_t x, size_t y) const {
			MLIB_ASSERT(x < m_width && y < m_height);
			return m_data[y*m_width + x];
		}

		//! Access Operator (int)
		const T& operator()(int x, int y) const {
			MLIB_ASSERT((unsigned int)x < m_width && (unsigned int)y < m_height);
			return m_data[y*m_width + x];
		}

        //! Access Operator (size_t)
        T& operator()(size_t x, size_t y) {
            MLIB_ASSERT(x < m_width && y < m_height);
            return m_data[y*m_width + x];
        }

		//! Access Operator (double); x,y \in [0;1]
		const T& operator()(double x, double y) const {
			return (*this)((unsigned int)round(x*(m_width - 1)), (unsigned int)round(y*(m_height - 1)));
		}

		//! Access Operator (float); x,y \in [0;1]
		const T& operator()(float x, float y) const {
			return (*this)((unsigned int)round(x*(m_width - 1)), (unsigned int)round(y*(m_height - 1)));
		}

		//! Returns the Pixel value at that position (calls the function corresponding to the parameter type)
		template <class S>
		const T& getPixel(S x, S y) const {
			return (*this)(x, y);
		}

		//! returns the bi-linearly interpolated pixel; S must be float or double; ; x,y \in [0;width/height[
		template<class S>
		T getInterpolated(S x, S y) const {
			static_assert(std::is_same<float, S>::value || std::is_same<double, S>::value, "must be double or float");

			unsigned int xl = math::floor(x);	unsigned int xh = math::ceil(x);
			unsigned int yl = math::floor(y);	unsigned int yh = math::ceil(y);

			S t = x - (S)xl;	//x interpolation parameter
			S s = y - (S)yl;	//y interpolation parameter

			T p0 = math::lerp(getPixel(xl, yl), getPixel(xh, yl), s);	// lerp between p_00 and p_10
			T p1 = math::lerp(getPixel(xl, yh), getPixel(xh, yh), s);	// lerp between P_01 and p_11
			return math::lerp(p0, p1, t);
		}


		//! Assignment operator
		BaseImage& operator=(const BaseImage& other) {
			if (this != &other) {
				if (other.m_width != m_width || other.m_height != m_height)	{
					SAFE_DELETE_ARRAY(m_data);
					create(other.m_width, other.m_height);
				}

				memcpy(m_data, other.m_data, sizeof(T) * m_height * m_width);
				m_InvalidValue = other.getInvalidValue();
				m_format = other.m_format;
			}
			return *this;
		}

		//! Assignment operator r-value
		BaseImage& operator=(BaseImage&& other) {
			swap(*this, other);
			return *this;
		}

		//! Comparison operator
		bool operator==(const BaseImage& other) {
			if (other.m_width != m_width || other.m_height != m_height) return false;
			for (unsigned int i = 0; i < m_width * m_height; i++) {
				if (m_data[i] != other.m_data[i])	return false;
			}
			return true;
		}

		//! Allocates data so that the current image and other have the same size
		void allocateSameSize(const BaseImage& other) {
			if (other.m_width != m_width || other.m_height != m_height) {
				SAFE_DELETE_ARRAY(m_data);
				create(other.m_width, other.m_height);
			}
		}

		//! Allocates the images to the given size
		void allocate(unsigned int width, unsigned int height) {
			if (width == 0 || height == 0) {
				free();
			}
			else if (m_width != width || m_height != height) {
				SAFE_DELETE_ARRAY(m_data);
				create(width, height);
			}
		}

		//! Copies a source image into a region of the current image
		void copyIntoImage(const BaseImage &source, unsigned int startX, unsigned int startY) {
			MLIB_ASSERT(source.getWidth() + startX <= getWidth() && source.getHeight() + startY <= getHeight());
			for (unsigned int y = startY; y < startY + source.getHeight(); y++) {
				for (unsigned int x = startX; x < startX + source.getWidth(); x++) {
					(*this)(x, y) = source(x - startX, y - startY);
				}
			}
		}

		//! Returns the width of the image
		unsigned int getWidth() const {
			return m_width;
		}

		//! Returns the height of the image
		unsigned int getHeight() const {
			return m_height;
		}

        //! Returns the dimensions of the image
        vec2i getDimensions() const {
            return vec2i(m_width, m_height);
        }

		//! Returns the number of pixels of the image
		unsigned int size() const {
			return m_width*m_height;
		}

		//! Returns the image data (linearized array)
		const T* getPointer() const {
			return m_data;
		}

		//! Returns the image data (linearized array)
		T* getPointer() {
			return m_data;
		}


		//! saves a file to a binary depth image (.mbindepth) or a binary color image (.binRGB); could be any bytes per pixel
		void saveAsBinaryMImage(const std::string &filename) const {
			saveBinaryMImage(filename, m_data, m_width, m_height);
		}

		//! saves a binary m image
		static void saveBinaryMImage(const std::string& filename, const void* data, unsigned int width, unsigned int height) {
			saveBinaryMImageArray(filename, &data, width, height, 1);
		}

		static void saveBinaryMImageArray(const std::string& filename, const std::vector<BaseImage<T>>& images) {
			MLIB_ASSERT(images.size() >= 1);
			T** data = new T*[images.size()];
			for (unsigned int i = 0; i < images.size(); i++) {
				MLIB_ASSERT(images[0].getWidth() == images[i].getWidth());
				MLIB_ASSERT(images[0].getHeight() == images[i].getHeight());
				data[i] = images[i].getDataPointer();
			}
			saveBinaryMImageArray(filename, data, images[0].getWidth(), images[0].getHeight(), images.size());
			SAFE_DELETE_ARRAY(data);
		}

		//! saves an array of binary m images
		static void saveBinaryMImageArray(const std::string& filename, const void** data, unsigned int width, unsigned int height, unsigned int numImages = 1) {
			if (util::getFileExtension(filename) != "mbindepth" && util::getFileExtension(filename) != "mbinRGB") throw MLIB_EXCEPTION("invalid file extension" + util::getFileExtension(filename));

			std::ofstream file(filename, std::ios::binary);
			if (!file.is_open())	throw std::ios::failure(__FUNCTION__ + std::string(": could not open file ") + filename);

			unsigned int bytesPerPixel = sizeof(T);
			file.write((char*)&numImages, sizeof(unsigned int));
			file.write((char*)&width, sizeof(unsigned int));
			file.write((char*)&height, sizeof(unsigned int));
			file.write((char*)&bytesPerPixel, sizeof(unsigned int));
			for (unsigned int i = 0; i < numImages; i++) {
				file.write((char*)data[i], width * height * bytesPerPixel);
			}
			file.close();
		}

		//! loads a file from a binary depth image (.mbindepth) or a binary color image (.binRGB)
		void loadFromBinaryMImage(const std::string& filename) {
			loadBinaryMImage(filename, (void**)&m_data, m_width, m_height);
		}

		static void loadBinaryMImage(const std::string& filename, void** data, unsigned int& width, unsigned int& height) {
			std::vector<void*> dataArray;
			loadBinaryMImageArray(filename, dataArray, width, height);
			MLIB_ASSERT(dataArray.size() == 1);
			SAFE_DELETE_ARRAY(data);
			*data = dataArray[0];
		}

		//! loads a binary array of m images
		static void loadBinaryMImageArray(const std::string& filename, std::vector<void*>& data, unsigned int& width, unsigned int& height) {
			if (util::getFileExtension(filename) != "mbindepth" && util::getFileExtension(filename) != "mbinRGB") throw MLIB_EXCEPTION("invalid file extension" + util::getFileExtension(filename));

			std::ifstream file(filename, std::ios::binary);
			if (!file.is_open())	throw std::ios::failure(__FUNCTION__ + std::string(": could not open file ") + filename);

			unsigned int bytesPerPixel, numImages;
			file.read((char*)&numImages, sizeof(unsigned int));
			file.read((char*)&width, sizeof(unsigned int));
			file.read((char*)&height, sizeof(unsigned int));
			file.read((char*)&bytesPerPixel, sizeof(unsigned int));
			assert(sizeof(T) == bytesPerPixel);
			for (unsigned int i = 0; i < numImages; i++) {
				void* currData = new T[width*height];
				file.read((char*)currData, width * height * bytesPerPixel);
				data.push_back(currData);
			}
			file.close();
		}

		//! counts the number of pixels not equal to value
		unsigned int getNumPixelsNotEqualTo(const T &value) {
			unsigned int count = 0;
			for (unsigned int i = 0; i < m_width * m_height; i++) {
				if (value != m_data[i])	count++;
			}
			return count;
		}

        //! fills the image with the given function of (x,y) pixel index
        void fill(const std::function< T(size_t, size_t) > &fillFunction)
        {
            for (size_t y = 0; y < m_height; y++)
                for (size_t x = 0; x < m_width; x++)
                {
                    (*this)(x, y) = fillFunction(x, y);
                }
        }

		//! sets all pixels with a specific value (oldValue); to a new value (newValue)
		void replacePixelValue(const T& oldValue, const T& newValue) {
			for (unsigned int i = 0; i < m_width * m_height; i++) {
				if (m_data[i] == oldValue)	m_data[i] = newValue;
			}
		}

		//! sets all pixels to value
		void setPixels(const T &value) {
			for (unsigned int i = 0; i < m_width * m_height; i++) {
				m_data[i] = value;
			}
		}

		//! flips the image vertically
		void flipY() {
			for (unsigned int y = 0; y < m_height / 2; y++) {
				for (unsigned int x = 0; x < m_width; x++) {
					T tmp = (*this)(x, y);
					(*this)(x, y) = (*this)(x, m_height - y - 1);
					(*this)(x, m_height - y - 1) = tmp;
				}
			}
		}

		//! flips the image horizontally
		void flipX() {
			for (unsigned int y = 0; y < m_height; y++) {
				for (unsigned int x = 0; x < m_width / 2; x++) {
					T tmp = (*this)(x, y);
					(*this)(x, y) = (*this)(m_width - x - 1, y);
					(*this)(m_width - x - 1, y) = tmp;
				}
			}
		}

		//! returns the invalid value
		T getInvalidValue() const {
			return m_InvalidValue;
		}

		//! sets the invalid value
		void setInvalidValue(T invalidValue) {
			m_InvalidValue = invalidValue;
		}

		//! sets a pixel to the invalid value
		template <class S>
		void setInvalid(S x, S y) {
			setPixel(x, y, getInvalidValue());
		}

		//! returns true if a value is valid
		bool isValidValue(T value) const {
			return value != m_InvalidValue;
		}

		//! returns true if the depth value at position (x,y) is valid
		template <class S>
		bool isValid(S x, S y) const {
			return getPixel(x, y) != m_InvalidValue;
		}

		bool isValidCoordinate(unsigned int x, unsigned int y) const {
			return (x < m_width && y < m_height);
		}

		//! returns the number of channels per pixel (-1 if unknown)
		unsigned int getNumChannels() const  {
			if (std::is_same<T, USHORT>::value || std::is_same<T, short >::value) return 1;
			if (std::is_same<T, double>::value || std::is_same<T, float >::value || std::is_same<T, UCHAR >::value || std::is_same<T, UINT  >::value || std::is_same<T, int   >::value) return 1;
			if (std::is_same<T, vec2d >::value || std::is_same<T, vec2f >::value || std::is_same<T, vec2uc>::value || std::is_same<T, vec2ui>::value || std::is_same<T, vec2i >::value) return 2;
			if (std::is_same<T, vec3d >::value || std::is_same<T, vec3f >::value || std::is_same<T, vec3uc>::value || std::is_same<T, vec3ui>::value || std::is_same<T, vec3i >::value) return 3;
			if (std::is_same<T, vec4d >::value || std::is_same<T, vec4f >::value || std::is_same<T, vec4uc>::value || std::is_same<T, vec4ui>::value || std::is_same<T, vec4i >::value) return 4;
			return (unsigned int)-1;
		}

		//! returns the number of bits per channel (-1 if unknown);
		unsigned int getNumBytesPerChannel() const  {
			const unsigned int numChannels = getNumChannels();
			if (numChannels != (unsigned int)-1) return sizeof(T) / numChannels;
			else return (unsigned int)-1;
		}

		//! returns the storage requirements per pixel
		unsigned int getNumBytesPerPixel() const {
			return sizeof(T);
		}

		////! computes the next mip map level of the image (box filtered image)
		//BaseImage mipMap(bool ignoreInvalidPixels = false) const {
		//	BaseImage result;
		//	mipMap(result, ignoreInvalidPixels);
		//	return result;
		//}

		//! computes the next mip map level of the image (box filtered image)
		void mipMap(BaseImage& result, bool ignoreInvalidPixels = false) const {
			result.allocate(m_width / 2, m_height / 2);
			result.setInvalidValue(m_InvalidValue);

			if (!ignoreInvalidPixels) {
				for (unsigned int y = 0; y < result.m_height; y++) {
					for (unsigned int x = 0; x < result.m_width; x++) {
						result(x, y) = getPixel(2 * x + 0, 2 * y + 0) + getPixel(2 * x + 1, 2 * y + 0) + getPixel(2 * x + 0, 2 * y + 1) + getPixel(2 * x + 1, 2 * y + 1);
						result(x, y) /= 4;
					}
				}
			}
			else {
				for (unsigned int y = 0; y < result.m_height; y++) {
					for (unsigned int x = 0; x < result.m_width; x++) {
						unsigned int valid = 0;
						T value = T();
						if (isValid(2 * x + 0, 2 * y + 0))	{
							valid++;
							value += getPixel(2 * x + 0, 2 * y + 0);
						}
						if (isValid(2 * x + 1, 2 * y + 0))	{
							valid++;
							value += getPixel(2 * x + 1, 2 * y + 0);
						}
						if (isValid(2 * x + 0, 2 * y + 1))	{
							valid++;
							value += getPixel(2 * x + 0, 2 * y + 1);
						}
						if (isValid(2 * x + 1, 2 * y + 1))	{
							valid++;
							value += getPixel(2 * x + 1, 2 * y + 1);
						}
						if (valid == 0) {
							result(x, y) = result.getInvalidValue();
						}
						else {
							result(x, y) = value / (float)valid;	//this cast is not ideal but works for most classes...
						}
					}
				}
			}
		}

		//! nearest neighbor re-sampling
		void reSample(unsigned int newWidth, unsigned int newHeight) {
			if (m_width != newWidth || m_height != newHeight) {
				BaseImage res(newWidth, newHeight);
				res.setInvalidValue(m_InvalidValue);
				for (unsigned int i = 0; i < newHeight; i++) {
					for (unsigned int j = 0; j < newWidth; j++) {
						const float x = (float)j / (newWidth - 1);
						const float y = (float)i / (newHeight - 1);
						res(j, i) = getPixel(x, y);
					}
				}
				swap(*this, res);
			}
		}


		//! smooth (laplacian smoothing step)
		void smooth(unsigned int steps = 1) {
			for (unsigned int i = 0; i < steps; i++) {
				BaseImage<T> other(m_width, m_height);
				other.setInvalidValue(m_InvalidValue);

				for (unsigned int y = 0; y < m_height; y++) {
					for (unsigned int x = 0; x < m_width; x++) {
						unsigned int valid = 0;
						T value = T();
						if (isValidCoordinate(x - 1, y + 0) && isValid(x - 1, y + 0))	{
							valid++;
							value += getPixel(x - 1, y + 0);
						}
						if (isValidCoordinate(x + 1, y + 0) && isValid(x + 1, y + 0))	{
							valid++;
							value += getPixel(x + 1, y + 0);
						}
						if (isValidCoordinate(x + 0, y + 1) && isValid(x + 0, y + 1))	{
							valid++;
							value += getPixel(x + 0, y + 1);
						}
						if (isValidCoordinate(x + 0, y - 1) && isValid(x + 0, y - 1))	{
							valid++;
							value += getPixel(x + 0, y - 1);
						}

						if (isValid(x, y)) {
							other.setPixel(x, y, ((float)valid*getPixel(x, y) + value) / (2 * (float)valid));
						}
					}
				}
				*this = other;
			}
		}


		//! various operator overloads
		template<class U>
		void scale(const U& s) {
			for (unsigned int i = 0; i < m_height*m_width; i++) {
				m_data[i] *= s;
			}
		}

		template<class U>
		BaseImage& operator*=(const U& s) {
			scale(s);
			return *this;
		}
		template<class U>
		BaseImage& operator/=(const U& s) {
			for (unsigned int i = 0; i < m_height*m_width; i++) {
				m_data[i] /= s;
			}
			return *this;
		}
		template<class U>
		BaseImage& operator+=(const U& s) {
			for (unsigned int i = 0; i < m_height*m_width; i++) {
				m_data[i] += s;
			}
			return *this;
		}
		template<class U>
		BaseImage& operator-=(const U& s) {
			for (unsigned int i = 0; i < m_height*m_width; i++) {
				m_data[i] -= s;
			}
			return *this;
		}

	protected:
		//! Allocates memory and sets the image size accordingly
		void create(unsigned int width, unsigned int height) {
			m_width = width;
			m_height = height;
			m_data = new T[m_height * m_width];
		}

		//! Image data
		T* m_data;

		//! Image width
		unsigned int m_height;

		//! Image height
		unsigned int m_width;

		//! Invalid image value
		T m_InvalidValue;

	};

	template<class BinaryDataBuffer, class BinaryDataCompressor, class T>
	inline BinaryDataStream<BinaryDataBuffer, BinaryDataCompressor>& operator<<(BinaryDataStream<BinaryDataBuffer, BinaryDataCompressor>& s, const BaseImage<T>& image) {
		s.writeData(image.getWidth());
		s.writeData(image.getHeight());
		s.writeData(image.getInvalidValue());
		s.writeData((BYTE*)image.getPointer(), sizeof(T)*image.getWidth()*image.getHeight());
		return s;
	}

	template<class BinaryDataBuffer, class BinaryDataCompressor, class T>
	inline BinaryDataStream<BinaryDataBuffer, BinaryDataCompressor>& operator>>(BinaryDataStream<BinaryDataBuffer, BinaryDataCompressor>& s, BaseImage<T>& image) {
		unsigned int width, height;
		T invalidValue;
		s.readData(&width);
		s.readData(&height);
		s.readData(&invalidValue);
		image.allocate(width, height);
		image.setInvalidValue(invalidValue);
		s.readData((BYTE*)image.getPointer(), sizeof(T)*width*height);
		return s;
	}

	class DepthImage16 : public BaseImage < unsigned short > {
	public:
		DepthImage16() : BaseImage() {
			m_format = Image::FORMAT_DepthImage16;
			m_InvalidValue = 0;
		}
		DepthImage16(unsigned int width, unsigned int height, const unsigned short *data) : BaseImage(width, height, data) {
			m_format = Image::FORMAT_DepthImage16;
			m_InvalidValue = 0;
		}
		DepthImage16(unsigned int width, unsigned int height) : BaseImage(width, height) {
			m_format = Image::FORMAT_DepthImage16;
			m_InvalidValue = 0;
		}

	private:
	};

	class DepthImage : public BaseImage < float > {
	public:
		DepthImage() : BaseImage() {
			m_format = Image::FORMAT_DepthImage;
			m_InvalidValue = -std::numeric_limits<float>::infinity();
		}

		DepthImage(unsigned int width, unsigned int height, const float *data) : BaseImage(width, height, data) {
			m_format = Image::FORMAT_DepthImage;
			m_InvalidValue = -std::numeric_limits<float>::infinity();
		}
		DepthImage(unsigned int width, unsigned int height) : BaseImage(width, height) {
			m_format = Image::FORMAT_DepthImage;
			m_InvalidValue = -std::numeric_limits<float>::infinity();
		}

		//! Saves the depth image as a PPM file; note that there is a loss of precision
		void saveAsPPM(const std::string &filename) const
		{
			std::ofstream out(filename, std::ofstream::out);

			if (out.fail())
			{
				std::cerr << "Error in function void __FUNCTION__ const: Can not open file " << filename << "!" << std::endl;
				exit(1);
			}

			out << "P3" << std::endl;
			out << "#" << filename << std::endl;
			out << m_height << " " << m_width << std::endl;

			out << "255" << std::endl;


			for (unsigned int y = 0; y < m_height; y++)	{
				for (unsigned int x = 0; x < m_width; x++)	{
					float res = getPixel(x, y);
					out <<
						(int)convertValueToExternalPPMFormat(res) << " " <<
						(int)convertValueToExternalPPMFormat(res) << " " <<
						(int)convertValueToExternalPPMFormat(res) << " " << "\n";
				}
			}

			out.close();
		}

		void loadFromPPM(const std::string &filename) {
			std::ifstream file(filename);
			if (!file.is_open()) {
				throw std::ifstream::failure(std::string("Could not open file!").append(filename));
			}

			std::string s;
			getline(file, s); // Format
			getline(file, s); // Comment
			getline(file, s); // Width and Height


			unsigned int width, height;
			std::stringstream wh(s);
			wh >> width;
			wh >> height;
			allocate(width, height);

			getline(file, s); // Max Value

			for (unsigned int y = 0; y < m_height; y++)	{
				for (unsigned int x = 0; x < m_width; x++)	{
					unsigned int c;
					vec3f color;

					file >> c; color.x = convertValueFromExternalPPMFormat((unsigned char)c);
					file >> c; color.y = convertValueFromExternalPPMFormat((unsigned char)c);
					file >> c; color.z = convertValueFromExternalPPMFormat((unsigned char)c);

					MLIB_ASSERT(c <= 255);
					MLIB_ASSERT(color.x == color.y && color.y == color.z);

					(*this)(x, y) = color.x;
				}
			}

			file.close();
		}


	private:
		static unsigned char convertValueToExternalPPMFormat(float x)
		{
			if (x < (float)0) {
				std::cout << __FUNCTION__ << ": Warning value clamped!" << std::endl;
				return 0;
			}

			if (x > (float)1)	{
				std::cout << __FUNCTION__ << ": Warning value clamped!" << std::endl;
				return 255;
			}

			return (unsigned char)(x*(float)255.0 + (float)0.49999);
		}

		static float convertValueFromExternalPPMFormat(unsigned char x)
		{
			return (float)x / 255.0f;
		}
	};


	class ColorImageRGB : public BaseImage < vec3f > {
	public:
		ColorImageRGB() : BaseImage() {
			m_format = Image::FORMAT_ColorImageR32G32B32;
			m_InvalidValue = vec3f(-std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity());
		}

		ColorImageRGB(unsigned int width, unsigned int height, const vec3f *data) : BaseImage(width, height, data) {
			m_format = Image::FORMAT_ColorImageR32G32B32;
			m_InvalidValue = vec3f(-std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity());
		}

		ColorImageRGB(unsigned int width, unsigned int height, const vec3uc *data, float scale = 255.0f) : BaseImage(width, height) {
			m_format = Image::FORMAT_ColorImageR32G32B32;
			m_InvalidValue = vec3f(-std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity());


			for (unsigned int y = 0; y < m_height; y++) {
				for (unsigned int x = 0; x < m_width; x++) {
					vec3f value((float)data[y*m_width + x].x / scale,
						(float)data[y*m_width + x].y / scale,
						(float)data[y*m_width + x].z / scale
						);
					setPixel(x, y, value);
				}
			}
		}
		ColorImageRGB(unsigned int width, unsigned int height) : BaseImage(width, height) {
			m_format = Image::FORMAT_ColorImageR32G32B32;
			m_InvalidValue = vec3f(-std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity());
		}

		ColorImageRGB(const DepthImage& depthImage, bool debugPrint = false) : BaseImage(depthImage.getWidth(), depthImage.getHeight()) {
			m_format = Image::FORMAT_ColorImageR32G32B32;
			m_InvalidValue = vec3f(-std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity());

			const float* data = depthImage.getPointer();
            float maxDepth = -std::numeric_limits<float>::max();
            float minDepth = std::numeric_limits<float>::max();
			for (unsigned int i = 0; i < getWidth()*getHeight(); i++) {
				if (data[i] != depthImage.getInvalidValue()) {
					if (data[i] > maxDepth) maxDepth = data[i];
					if (data[i] < minDepth) minDepth = data[i];
				}
			}
			if (debugPrint) {
				std::cout << "max Depth " << maxDepth << std::endl;
				std::cout << "min Depth " << minDepth << std::endl;
			}
			for (unsigned int i = 0; i < getWidth()*getHeight(); i++) {
				if (data[i] != depthImage.getInvalidValue()) {
					m_data[i] = BaseImageHelper::convertDepthToRGB(data[i], minDepth, maxDepth);
				}
				else {
					m_data[i] = getInvalidValue();
				}
			}
		}
		ColorImageRGB(const DepthImage& depthImage, float minDepth, float maxDepth) : BaseImage(depthImage.getWidth(), depthImage.getHeight()) {
			m_format = Image::FORMAT_ColorImageR32G32B32;
			m_InvalidValue = vec3f(-std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity());

			const float* data = depthImage.getPointer();
			for (unsigned int i = 0; i < getWidth()*getHeight(); i++) {
				if (data[i] != depthImage.getInvalidValue()) {
					m_data[i] = BaseImageHelper::convertDepthToRGB(data[i], minDepth, maxDepth);
				}
				else {
					m_data[i] = getInvalidValue();
				}
			}
		}
		ColorImageRGB(const BaseImage<float>& image) : BaseImage(image.getWidth(), image.getHeight()) {
			m_format = Image::FORMAT_ColorImageR32G32B32;
			m_InvalidValue = vec3f(-std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity());

			const float* data = image.getPointer();
			for (unsigned int i = 0; i < getWidth()*getHeight(); i++) {
				m_data[i] = ml::vec3f(data[i]);
			}
		}
	};


	class ColorImageRGBA : public BaseImage < vec4f > {
	public:
		ColorImageRGBA() : BaseImage() {
			m_format = Image::FORMAT_ColorImageR32G32B32A32;
			m_InvalidValue = vec4f(-std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity());
		}

		ColorImageRGBA(unsigned int width, unsigned int height, const vec4f *data) : BaseImage(width, height, data) {
			m_format = Image::FORMAT_ColorImageR32G32B32A32;
			m_InvalidValue = vec4f(-std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity());
		}

		ColorImageRGBA(unsigned int width, unsigned int height, const vec4uc *data, float scale = 255.0f) : BaseImage(width, height) {
			m_format = Image::FORMAT_ColorImageR32G32B32A32;
			m_InvalidValue = vec4f(-std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity());

			for (unsigned int y = 0; y < m_height; y++) {
				for (unsigned int x = 0; x < m_width; x++) {
					vec4f value(
						(float)data[y*m_width + x].x / scale,
						(float)data[y*m_width + x].y / scale,
						(float)data[y*m_width + x].z / scale,
						(float)data[y*m_width + x].w / scale
						);
					setPixel(x, y, value);
				}
			}
		}
		ColorImageRGBA(unsigned int width, unsigned int height) : BaseImage(width, height) {
			m_format = Image::FORMAT_ColorImageR32G32B32A32;
			m_InvalidValue = vec4f(-std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity());
		}

		ColorImageRGBA(const DepthImage& depthImage, bool debugPrint = false) : BaseImage(depthImage.getWidth(), depthImage.getHeight()) {
			m_format = Image::FORMAT_ColorImageR32G32B32A32;
			m_InvalidValue = vec4f(-std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity());

			const float* data = depthImage.getPointer();
            float maxDepth = -std::numeric_limits<float>::max();
            float minDepth = +std::numeric_limits<float>::max();
			for (unsigned int i = 0; i < getWidth()*getHeight(); i++) {
				if (data[i] != depthImage.getInvalidValue()) {
					if (data[i] > maxDepth) maxDepth = data[i];
					if (data[i] < minDepth) minDepth = data[i];
				}
			}
			if (debugPrint) {
				std::cout << "max Depth " << maxDepth << std::endl;
				std::cout << "min Depth " << minDepth << std::endl;
			}
			for (unsigned int i = 0; i < getWidth()*getHeight(); i++) {
				if (data[i] != depthImage.getInvalidValue()) {
					m_data[i] = BaseImageHelper::convertDepthToRGBA(data[i], minDepth, maxDepth);
				}
				else {
					m_data[i] = getInvalidValue();
				}
			}
		}
	};

	typedef ColorImageRGB	PointImage;
	typedef ColorImageRGB	ColorImageR32G32B32;
	typedef ColorImageRGBA	ColorImageR32G32B32A32;

	typedef BaseImage<float>	ColorImageR32;
	typedef BaseImage<vec3uc>	ColorImageR8G8B8;
	typedef BaseImage<vec4uc>	ColorImageR8G8B8A8;


} // namespace ml


#endif

