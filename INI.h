#pragma once

class CIniReader  
{
public:
	// method to set INI file name, if not already specified 
	void setINIFileName(CString strINIFile);

	// methods to return the lists of section data and section names
	CStringList* getSectionData(CString strSection);
	CStringList* getSectionNames();

	// check if the section exists in the file
	BOOL sectionExists(CString strSection);

	// updates the key value, if key already exists, else creates a key-value pair
	long setKey(CString strValue, CString strKey, CString strSection);

	// give the key value for the specified key of a section
	CString getKeyValue(CString strKey,CString strSection);
	// get value as int and automatically detect hexadecimal numbers by 0x prefix
	unsigned int getKeyValueAsUInt(CString strKey,CString strSection,unsigned int DefaultValue = -1);

	// default constructor
	CIniReader()
	{
		m_sectionList = new CStringList();
		m_sectionDataList = new CStringList();
	}

	CIniReader(CString strFile)
	{
		m_strFileName = strFile;
		m_sectionList = new CStringList();
		m_sectionDataList = new CStringList();
	}

	~CIniReader()
	{
		delete m_sectionList;
		delete m_sectionDataList;
	}

private:
	// lists to keep sections and section data
	CStringList *m_sectionDataList;
	CStringList *m_sectionList;

	CString m_strSection;
	long m_lRetValue;

	// ini file name 
	CString m_strFileName;
};
