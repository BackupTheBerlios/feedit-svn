#pragma once

class TreeData
{
public:
	virtual ATL::CString ToString() = 0;
};

class FolderData : public TreeData
{
public:
	ATL::CString ToString()
	{
		return "Folder";
	}

	ATL::CString m_name;
};

class FeedData : public TreeData
{
public:
	ATL::CString ToString()
	{
		return "Feed";
	}

	ATL::CString m_name;
	ATL::CString m_url;
};
