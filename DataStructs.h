#pragma once

class TreeData
{
public:
	virtual CAtlString ToString() = 0;

	int m_id;
};

class FolderData : public TreeData
{
public:
	CAtlString ToString()
	{
		return "Folder";
	}

	CAtlString m_name;
};

class FeedData : public TreeData
{
public:
	CAtlString ToString()
	{
		return "Feed";
	}

	CAtlString m_title;
	CAtlString m_link;
	CAtlString m_description;
	int m_unread;
	int m_navigateURL;
};

class ListData
{
public:
	virtual CAtlString ToString() = 0;

	int m_id;
	CAtlString m_url;
};

class NewsData : public ListData
{
public:
	CAtlString ToString()
	{
		return "News";
	}

	CAtlString m_issued;
	CAtlString m_title;
	CAtlString m_description;
	bool m_unread;
	bool m_flagged;
	HTREEITEM m_feedTreeItem;
};
