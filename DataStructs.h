#pragma once

class TreeData
{
public:
	virtual ATL::CString ToString() = 0;

	int m_id;
	ATL::CString m_name;
};

class FolderData : public TreeData
{
public:
	ATL::CString ToString()
	{
		return "Folder";
	}
};

class FeedData : public TreeData
{
public:
	ATL::CString ToString()
	{
		return "Feed";
	}

	ATL::CString m_url;
};
