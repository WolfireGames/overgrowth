// This code is in the public domain -- castanyo@yahoo.es

#ifndef NV_CORE_REFCOUNTED_H
#define NV_CORE_REFCOUNTED_H

#include <nvcore/nvcore.h>
#include <nvcore/Debug.h>


namespace nv
{

	/// Reference counted base class to be used with SmartPtr and WeakPtr.
	class RefCounted
	{
		NV_FORBID_COPY(RefCounted);
	public:

		/// Ctor.
		RefCounted() : m_count(0)/*, m_weak_proxy(NULL)*/
		{
			s_total_obj_count++;
		}

		/// Virtual dtor.
		virtual ~RefCounted()
		{
			nvCheck( m_count == 0 );
			nvCheck( s_total_obj_count > 0 );
			s_total_obj_count--;
		}


		/// Increase reference count.
		uint addRef() const
		{
			s_total_ref_count++;
			m_count++;
			return m_count;
		}


		/// Decrease reference count and remove when 0.
		uint release() const
		{
			nvCheck( m_count > 0 );
			
			s_total_ref_count--;
			m_count--;
			if( m_count == 0 ) {
			//	releaseWeakProxy();
				delete this;
				return 0;
			}
			return m_count;
		}
	/*
		/// Get weak proxy.
		WeakProxy * getWeakProxy() const
		{
			if (m_weak_proxy == NULL) {
				m_weak_proxy = new WeakProxy;
				m_weak_proxy->AddRef();
			}
			return m_weak_proxy;
		}

		/// Release the weak proxy.	
		void releaseWeakProxy() const
		{
			if (m_weak_proxy != NULL) {
				m_weak_proxy->NotifyObjectDied();
				m_weak_proxy->Release();
				m_weak_proxy = NULL;
			}
		}
	*/
		/** @name Debug methods: */
		//@{
			/// Get reference count.
			int refCount() const
			{
				return m_count;
			}

			/// Get total number of objects.
			static int totalObjectCount()
			{
				return s_total_obj_count;
			}

			/// Get total number of references.
			static int totalReferenceCount()
			{
				return s_total_ref_count;
			}
		//@}


	private:

		NVCORE_API static int s_total_ref_count;
		NVCORE_API static int s_total_obj_count;

		mutable int m_count;
	//	mutable WeakProxy * weak_proxy;

	};


} // nv namespace


#endif // NV_CORE_REFCOUNTED_H
