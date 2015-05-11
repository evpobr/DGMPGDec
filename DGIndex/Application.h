#pragma once

#include "DGIndex_h.h"

class CApplication : public IApplication
{
public:
	STDMETHODIMP QueryInterface(REFIID riid, void **ppvObject);
	STDMETHODIMP_(ULONG) AddRef(void);
	STDMETHODIMP_(ULONG) Release(void);

	STDMETHODIMP LoadProject(LPCTSTR pszFileName);
	STDMETHODIMP Quit();

private:
	ULONG m_cRef;
};