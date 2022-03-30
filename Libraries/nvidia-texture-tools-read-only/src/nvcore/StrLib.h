// This code is in the public domain -- castanyo@yahoo.es

#ifndef NV_CORE_STRING_H
#define NV_CORE_STRING_H

#include <nvcore/nvcore.h>
#include <nvcore/Containers.h>	// swap

#include <string.h> // strlen, strcmp, etc.


namespace nv
{

	uint strHash(const char * str, uint h) NV_PURE;

	/// String hash based on Bernstein's hash.
	inline uint strHash(const char * data, uint h = 5381)
	{
		uint i;
		while(data[i] != 0) {
			h = (33 * h) ^ uint(data[i]);
			i++;
		}
		return h;
	}
	
	template <> struct hash<const char *> {
		uint operator()(const char * str) const { return strHash(str); }
	};
	
	NVCORE_API int strCaseCmp(const char * s1, const char * s2) NV_PURE;
	NVCORE_API int strCmp(const char * s1, const char * s2) NV_PURE;
	NVCORE_API void strCpy(char * dst, int size, const char * src);
	NVCORE_API void strCpy(char * dst, int size, const char * src, int len);
	NVCORE_API void strCat(char * dst, int size, const char * src);

	NVCORE_API bool strMatch(const char * str, const char * pat) NV_PURE;

	
	/// String builder.
	class NVCORE_CLASS StringBuilder
	{
	public:
	
		StringBuilder();
		explicit StringBuilder( int size_hint );
		StringBuilder( const char * str );
		StringBuilder( const StringBuilder & );
	
		~StringBuilder();
	
		StringBuilder & format( const char * format, ... ) __attribute__((format (printf, 2, 3)));
		StringBuilder & format( const char * format, va_list arg );
	
		StringBuilder & append( const char * str );
		StringBuilder & appendFormat( const char * format, ... ) __attribute__((format (printf, 2, 3)));
		StringBuilder & appendFormat( const char * format, va_list arg );
	
		StringBuilder & number( int i, int base = 10 );
		StringBuilder & number( uint i, int base = 10 );
	
		StringBuilder & reserve( uint size_hint );
		StringBuilder & copy( const char * str );
		StringBuilder & copy( const StringBuilder & str );
		
		StringBuilder & toLower();
		StringBuilder & toUpper();
		
		void reset();
		bool isNull() const { return m_size == 0; }
	
		// const char * accessors
		operator const char * () const { return m_str; }
		operator char * () { return m_str; }
		const char * str() const { return m_str; }
		char * str() { return m_str; }
	
		/// Implement value semantics.
		StringBuilder & operator=( const StringBuilder & s ) {
			return copy(s);
		}

		/// Implement value semantics.
		StringBuilder & operator=( const char * s ) {
			return copy(s);
		}

		/// Equal operator.
		bool operator==( const StringBuilder & s ) const {
			if (s.isNull()) return isNull();
			else if (isNull()) return false;
			else return strcmp(s.m_str, m_str) != 0;
		}
		
		/// Return the exact length.
		uint length() const { return isNull() ? 0 : uint(strlen(m_str)); }
	
		/// Return the size of the string container.
		uint capacity() const { return m_size; }
	
		/// Return the hash of the string.
		uint hash() const { return isNull() ? 0 : strHash(m_str); }
	
		///	Swap strings.
		friend void swap(StringBuilder & a, StringBuilder & b) {
			nv::swap(a.m_size, b.m_size);
			nv::swap(a.m_str, b.m_str);
		}
	
	protected:
		
		/// Size of the string container.
		uint m_size;
		
		/// String.
		char * m_str;
		
	};


	/// Path string. @@ This should be called PathBuilder.
	class NVCORE_CLASS Path : public StringBuilder
	{
	public:
		Path() : StringBuilder() {}
		explicit Path(int size_hint) : StringBuilder(size_hint) {}
		Path(const char * str) : StringBuilder(str) {}
		Path(const Path & path) : StringBuilder(path) {}
		
		const char * fileName() const;
		const char * extension() const;
		
		void translatePath();
		
		void stripFileName();
		void stripExtension();

		// statics
		static char separator();
		static const char * fileName(const char *);
		static const char * extension(const char *);
	};
	
	
	/// String class.
	class NVCORE_CLASS String
	{
	public:

		/// Constructs a null string. @sa isNull()
		String()
		{
			data = s_null.data;
			addRef();
		}

		/// Constructs a shared copy of str.
		String(const String & str)
		{
			data = str.data;
			addRef();
		}

		/// Constructs a shared string from a standard string.
		String(const char * str)
		{
			setString(str);
		}

		/// Constructs a shared string from a standard string.
		String(const char * str, int length)
		{
			setString(str, length);
		}

		/// Constructs a shared string from a StringBuilder.
		String(const StringBuilder & str)
		{
			setString(str);
		}

		/// Dtor.
		~String()
		{
			nvDebugCheck(data != NULL);
			release();
		}

		String clone() const;
	
		/// Release the current string and allocate a new one.
		const String & operator=( const char * str )
		{
			release();
			setString( str );
			return *this;
		}

		/// Release the current string and allocate a new one.
		const String & operator=( const StringBuilder & str )
		{
			release();
			setString( str );
			return *this;
		}
	
		/// Implement value semantics.
		String & operator=( const String & str )
		{
			if (str.data != data)
			{
				release();
				data = str.data;
				addRef();
			}
			return *this;
		}

		/// Equal operator.
		bool operator==( const String & str ) const
		{
			nvDebugCheck(data != NULL);
			nvDebugCheck(str.data != NULL);
			if( str.data == data ) {
				return true;
			}
			return strcmp(data, str.data) == 0;
		}

		/// Equal operator.
		bool operator==( const char * str ) const
		{
			nvDebugCheck(data != NULL);
			nvCheck(str != NULL);	// Use isNull!
			return strcmp(data, str) == 0;
		}

		/// Not equal operator.
		bool operator!=( const String & str ) const
		{
			nvDebugCheck(data != NULL);
			nvDebugCheck(str.data != NULL);
			if( str.data == data ) {
				return false;
			}
			return strcmp(data, str.data) != 0;
		}
	
		/// Not equal operator.
		bool operator!=( const char * str ) const
		{
			nvDebugCheck(data != NULL);
			nvCheck(str != NULL);	// Use isNull!
			return strcmp(data, str) != 0;
		}
	
		/// Returns true if this string is the null string.
		bool isNull() const { nvDebugCheck(data != NULL); return data == s_null.data; }
	
		/// Return the exact length.
		uint length() const { nvDebugCheck(data != NULL); return uint(strlen(data)); }
	
		/// Return the hash of the string.
		uint hash() const { nvDebugCheck(data != NULL); return strHash(data); }
	
		/// const char * cast operator.
		operator const char * () const { nvDebugCheck(data != NULL); return data; }
	
		/// Get string pointer.
		const char * str() const { nvDebugCheck(data != NULL); return data; }
	

	private:

		enum null_t { null };
		
		// Private constructor for null string.
		String(null_t) {
			setString("");
		}

		// Add reference count.
		void addRef() {
			nvDebugCheck(data != NULL);
			setRefCount(getRefCount() + 1);
		}
		
		// Decrease reference count.
		void release() {
			nvDebugCheck(data != NULL);

			const uint16 count = getRefCount();
			setRefCount(count - 1);
			if( count - 1 == 0 ) {
				free(data - 2);
				data = NULL;
			}
		}
		
		uint16 getRefCount() const {
			return *reinterpret_cast<const uint16 *>(data - 2);
		}
		
		void setRefCount(uint16 count) {
			nvCheck(count < 0xFFFF);
			*reinterpret_cast<uint16 *>(const_cast<char *>(data - 2)) = uint16(count);
		}
		
		void setData(const char * str) {
			data = str + 2;
		}
		
		void allocString(const char * str)
		{
			allocString(str, (int)strlen(str));
		}

		void allocString(const char * str, int len)
		{
			const char * ptr = malloc<char>(2 + len + 1);
	
			setData( ptr );				
			setRefCount( 0 );
			
			// Copy string.
			strCpy(const_cast<char *>(data), len+1, str, len);

			// Add terminating character.
			const_cast<char *>(data)[len] = '\0';
		}
	
		void setString(const char * str);
		void setString(const char * str, int length);
		void setString(const StringBuilder & str);	
	
		///	Swap strings.
		friend void swap(String & a, String & b) {
			swap(a.data, b.data);
		}
	
	private:

		NVCORE_API static String s_null;

		const char * data;
		
	};

} // nv namespace

#endif // NV_CORE_STRING_H
