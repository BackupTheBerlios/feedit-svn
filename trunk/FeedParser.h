#pragma once

class CFeedParser
{
public:
	enum FeedType
	{
		FPFT_UNKNOWN,
		FPFT_RSS09,
		FPFT_RSS10,
		FPFT_RSS20,
		FPFT_ATOM
	};

	struct FeedItem
	{
		CAtlString m_title;
		CAtlString m_url;
		CAtlString m_description;
		CAtlString m_date;
	};

	CFeedParser(const char* url) : m_type(FPFT_UNKNOWN)
	{
		ParseFeed(url);
	}

	FeedType ParseFeed(const char* url)
	{
		MSXML2::IXMLDOMDocument2Ptr xmldocument;
		xmldocument.CreateInstance(_uuidof(MSXML2::DOMDocument));
		ATLASSERT(xmldocument != NULL);
		xmldocument->async = FALSE;
		xmldocument->setProperty(_bstr_t("SelectionLanguage"), _variant_t("XPath"));
		xmldocument->setProperty(_bstr_t("SelectionNamespaces"), _variant_t("xmlns:rss09=\"http://my.netscape.com/rdf/simple/0.9/\" xmlns:rss10=\"http://purl.org/rss/1.0/\" xmlns:rdf=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\" xmlns:dc=\"http://purl.org/dc/elements/1.1/\" xmlns:atom=\"http://purl.org/atom/ns#\""));
		xmldocument->load(_variant_t(url));

		if(xmldocument->parseError->errorCode != 0)
		{
			m_error = (char*)xmldocument->parseError->reason;
			return FPFT_UNKNOWN;
		}

		MSXML2::IXMLDOMNodePtr node;

		if((node = xmldocument->selectSingleNode(_bstr_t("/rdf:RDF/rss09:channel"))) != NULL)
		{
			m_type = FPFT_RSS09;
			MSXML2::IXMLDOMNodePtr titlenode = node->selectSingleNode(_bstr_t("rss09:title"));
			MSXML2::IXMLDOMNodePtr linknode = node->selectSingleNode(_bstr_t("rss09:link"));
			MSXML2::IXMLDOMNodePtr imagenode = node->selectSingleNode(_bstr_t("rss09:image/@rdf:resource"));
			MSXML2::IXMLDOMNodePtr descriptionnode = node->selectSingleNode(_bstr_t("rss09:description"));

			if(titlenode != NULL)
			{
				m_title = (LPTSTR)titlenode->text;
				HTMLUnencode(m_title);
			}

			if(linknode != NULL)
				m_link = (LPTSTR)linknode->text;

			if(imagenode != NULL)
				m_image = (LPTSTR)imagenode->text;

			if(descriptionnode != NULL)
				m_description = (LPTSTR)descriptionnode->text;
		}
		else if((node = xmldocument->selectSingleNode(_bstr_t("/rdf:RDF/rss10:channel"))) != NULL)
		{
			m_type = FPFT_RSS10;
			MSXML2::IXMLDOMNodePtr titlenode = node->selectSingleNode(_bstr_t("rss10:title"));
			MSXML2::IXMLDOMNodePtr linknode = node->selectSingleNode(_bstr_t("rss10:link"));
			MSXML2::IXMLDOMNodePtr imagenode = node->selectSingleNode(_bstr_t("rss10:image/@rdf:resource"));
			MSXML2::IXMLDOMNodePtr descriptionnode = node->selectSingleNode(_bstr_t("rss10:description"));

			if(titlenode != NULL)
			{
				m_title = (LPTSTR)titlenode->text;
				HTMLUnencode(m_title);
			}

			if(linknode != NULL)
				m_link = (LPTSTR)linknode->text;

			if(imagenode != NULL)
				m_image = (LPTSTR)imagenode->text;

			if(descriptionnode != NULL)
				m_description = (LPTSTR)descriptionnode->text;
		}
		else if((node = xmldocument->selectSingleNode(_bstr_t("/rss/channel"))) != NULL)
		{
			m_type = FPFT_RSS20;
			MSXML2::IXMLDOMNodePtr titlenode = node->selectSingleNode(_bstr_t("title"));
			MSXML2::IXMLDOMNodePtr linknode = node->selectSingleNode(_bstr_t("link"));
			MSXML2::IXMLDOMNodePtr imagenode = node->selectSingleNode(_bstr_t("image/url"));
			MSXML2::IXMLDOMNodePtr descriptionnode = node->selectSingleNode(_bstr_t("description"));

			if(titlenode != NULL)
			{
				m_title = (LPTSTR)titlenode->text;
				HTMLUnencode(m_title);
			}

			if(linknode != NULL)
				m_link = (LPTSTR)linknode->text;

			if(imagenode != NULL)
				m_image = (LPTSTR)imagenode->text;

			if(descriptionnode != NULL)
				m_description = (LPTSTR)descriptionnode->text;
		}
		else if((node = xmldocument->selectSingleNode(_bstr_t("/atom:feed"))) != NULL)
		{
			m_type = FPFT_ATOM;
			MSXML2::IXMLDOMNodePtr titlenode = node->selectSingleNode(_bstr_t("atom:title"));
			MSXML2::IXMLDOMNodePtr linknode = node->selectSingleNode(_bstr_t("atom:link[@rel=\"alternate\"]/@href"));
			MSXML2::IXMLDOMNodePtr imagenode = node->selectSingleNode(_bstr_t("atom:image/atom:link[@rel=\"alternate\"]/@href"));
			MSXML2::IXMLDOMNodePtr descriptionnode = node->selectSingleNode(_bstr_t("atom:tagline"));

			if(titlenode != NULL)
			{
				m_title = (LPTSTR)titlenode->text;
				HTMLUnencode(m_title);
			}

			if(linknode != NULL)
				m_link = (LPTSTR)linknode->text;

			if(imagenode != NULL)
				m_image = (LPTSTR)imagenode->text;

			if(descriptionnode != NULL)
				m_description = (LPTSTR)descriptionnode->text;
		}

		switch(m_type)
		{
		case FPFT_RSS09:
			{
				MSXML2::IXMLDOMNodeListPtr nodes = xmldocument->selectNodes(_bstr_t("/rdf:RDF/rss09:item"));

				if(nodes != NULL && nodes->length > 0)
				{
					MSXML2::IXMLDOMNodePtr node;

					while((node = nodes->nextNode()) != NULL)
					{
						MSXML2::IXMLDOMNodePtr titlenode = node->selectSingleNode(_bstr_t("rss09:title"));
						MSXML2::IXMLDOMNodePtr urlnode = node->selectSingleNode(_bstr_t("rss09:link"));
						MSXML2::IXMLDOMNodePtr descriptionnode = node->selectSingleNode(_bstr_t("rss09:description"));
						MSXML2::IXMLDOMNodePtr datenode = node->selectSingleNode(_bstr_t("dc:date"));
						FeedItem item;

						if(titlenode != NULL)
						{
							item.m_title = (LPTSTR)titlenode->text;
							HTMLUnencode(item.m_title);
						}

						if(urlnode != NULL)
							item.m_url = (LPTSTR)urlnode->text;

						if(descriptionnode != NULL)
							item.m_description = (LPTSTR)descriptionnode->text;

						if(datenode != NULL)
							item.m_date = (LPTSTR)datenode->text;

						m_items.Add(item);
					}
				}

				break;
			}
		case FPFT_RSS10:
			{
				MSXML2::IXMLDOMNodeListPtr nodes = xmldocument->selectNodes(_bstr_t("/rdf:RDF/rss10:item"));

				if(nodes != NULL && nodes->length > 0)
				{
					MSXML2::IXMLDOMNodePtr node;

					while((node = nodes->nextNode()) != NULL)
					{
						MSXML2::IXMLDOMNodePtr titlenode = node->selectSingleNode(_bstr_t("rss10:title"));
						MSXML2::IXMLDOMNodePtr urlnode = node->selectSingleNode(_bstr_t("rss10:link"));
						MSXML2::IXMLDOMNodePtr descriptionnode = node->selectSingleNode(_bstr_t("rss10:description"));
						MSXML2::IXMLDOMNodePtr datenode = node->selectSingleNode(_bstr_t("dc:date"));
						FeedItem item;

						if(titlenode != NULL)
						{
							item.m_title = (LPTSTR)titlenode->text;
							HTMLUnencode(item.m_title);
						}

						if(urlnode != NULL)
							item.m_url = (LPTSTR)urlnode->text;

						if(descriptionnode != NULL)
							item.m_description = (LPTSTR)descriptionnode->text;

						if(datenode != NULL)
							item.m_date = (LPTSTR)datenode->text;

						m_items.Add(item);
					}
				}

				break;
			}
		case FPFT_RSS20:
			{
				MSXML2::IXMLDOMNodeListPtr nodes = xmldocument->selectNodes(_bstr_t("/rss/channel/item"));

				if(nodes != NULL && nodes->length > 0)
				{
					MSXML2::IXMLDOMNodePtr node;

					while((node = nodes->nextNode()) != NULL)
					{
						MSXML2::IXMLDOMNodePtr titlenode = node->selectSingleNode(_bstr_t("title"));
						MSXML2::IXMLDOMNodePtr urlnode = node->selectSingleNode(_bstr_t("link"));
						MSXML2::IXMLDOMNodePtr descriptionnode = node->selectSingleNode(_bstr_t("description"));
						MSXML2::IXMLDOMNodePtr datenode = node->selectSingleNode(_bstr_t("date"));

						if(datenode == NULL)
							datenode = node->selectSingleNode(_bstr_t("dc:date"));

						if(datenode == NULL)
							datenode = node->selectSingleNode(_bstr_t("pubDate"));

						FeedItem item;

						if(titlenode != NULL)
						{	
							item.m_title = (LPTSTR)titlenode->text;
							HTMLUnencode(item.m_title);
						}

						if(urlnode != NULL)
							item.m_url = (LPTSTR)urlnode->text;

						if(descriptionnode != NULL)
							item.m_description = (LPTSTR)descriptionnode->text;

						if(datenode != NULL)
							item.m_date = (LPTSTR)datenode->text;

						m_items.Add(item);
					}
				}

				break;
			}
		case FPFT_ATOM:
			{
				MSXML2::IXMLDOMNodeListPtr nodes = xmldocument->selectNodes(_bstr_t("/atom:feed/atom:entry"));

				if(nodes != NULL && nodes->length > 0)
				{
					MSXML2::IXMLDOMNodePtr node;

					while((node = nodes->nextNode()) != NULL)
					{
						MSXML2::IXMLDOMNodePtr titlenode = node->selectSingleNode(_bstr_t("atom:title"));
						MSXML2::IXMLDOMNodePtr urlnode = node->selectSingleNode(_bstr_t("atom:link[@rel=\"alternate\"]/@href"));
						MSXML2::IXMLDOMNodePtr descriptionnode = node->selectSingleNode(_bstr_t("atom:content"));
						MSXML2::IXMLDOMNodePtr datenode = node->selectSingleNode(_bstr_t("atom:issued"));
						FeedItem item;

						if(titlenode != NULL)
						{
							item.m_title = (LPTSTR)titlenode->text;
							HTMLUnencode(item.m_title);
						}

						if(urlnode != NULL)
							item.m_url = (LPTSTR)urlnode->text;

						if(descriptionnode != NULL)
							item.m_description = (LPTSTR)descriptionnode->text;

						if(datenode != NULL)
							item.m_date = (LPTSTR)datenode->text;

						m_items.Add(item);
					}
				}

				break;
			}
		}

		return m_type;
	}
	
	CAtlString AutoDiscoverFeed(const char* url)
	{
		CAtlString strURLBase = url;
		if (strURLBase.GetLength() < 1)
			return "";

		if (strURLBase.GetAt(strURLBase.GetLength()-1) != '/')
			strURLBase += "/";

		CAtlArray<CAtlString> arry;
		arry.Add(strURLBase+"atom.xml");
		arry.Add(strURLBase+"wp-rss2.php");
		arry.Add(strURLBase+"index.rss");
		arry.Add(strURLBase+"index.rdf");
		arry.Add(strURLBase+"rss.aspx");
		arry.Add(strURLBase+"Rss.aspx");
		arry.Add(strURLBase+"feed.rss");
		arry.Add(strURLBase+"rss.php");
		arry.Add(strURLBase+"feed.xml");
		arry.Add(strURLBase+"feed/rss2/");
		arry.Add(strURLBase+"rss2/");
		arry.Add(strURLBase+"index.xml");
		
		for (int i = 0 ; i < arry.GetCount();++i)
		{
			if ( ParseFeed(arry.GetAt(i)) != FPFT_UNKNOWN)
				return arry.GetAt(i);
		}

		return "";
		
	}

	void HTMLUnencode(CAtlString& s)
	{
		CAtlString sRet;

		for(size_t i = 0; i < s.GetLength(); ++i)
		{
			if(s.GetString()[i] == '&')
			{
				if(s.GetLength() - i >= 6)
				{
					if(s.GetString()[i] == '&' &&
						s.GetString()[i+1] == '#' &&
						isdigit(s.GetString()[i+2]) &&
						isdigit(s.GetString()[i+3]) &&
						isdigit(s.GetString()[i+4]) &&
						s.GetString()[i+5] == ';')
					{
						sRet += (char)( 
							((s.GetString()[i+2] - '0') * 100) +
						    ((s.GetString()[i+3] - '0') * 10) +
						    ((s.GetString()[i+4] - '0') ));
						i += 5;
						continue;
					}
				}
				if(s.GetLength() - i >= 6)
				{
					if(memcmp(&s.GetString()[i], "&apos;", 6) == 0)
					{
						sRet += '\'';
						i += 5;
						continue;
					}
					if(memcmp(&s.GetString()[i], "&nbsp;", 6) == 0)
					{
						sRet += ' ';
						i += 5;
						continue;
					}
					if(memcmp(&s.GetString()[i], "&quot;", 6) == 0)
					{
						sRet += '\"';
						i += 5;
						continue;
					}
				}
				if(s.GetLength() - i >= 5 && memcmp(&s.GetString()[i], "&amp;", 5) == 0)
				{
					sRet += '&';
					i += 4; //todo:  We should re-scan like this:
					//                    wxTmemmove((wxChar*)&s.c_str()[i+1],
					//                                (wxChar*)&s.c_str()[i+5],
					//                                s.length() - (i+5) );
					//                    s.c_str()[i+5]
					//                    i -= 1;
					continue;
				}
				if(s.GetLength() - i >= 4)
				{
					if(memcmp(&s.GetString()[i], "&lt;", 4) == 0)
					{
						sRet += '<';
						i += 3;
						continue;
					}
					if(memcmp(&s.GetString()[i], "&gt;", 4) == 0)
					{
						sRet += '>';
						i += 3;
						continue;
					}
				}
			}

			sRet += s.GetString()[i];
		}

		s = sRet;
	} 

	FeedType m_type;
	CAtlString m_error;
	CAtlString m_title;
	CAtlString m_link;
	CAtlString m_image;
	CAtlString m_description;
	CAtlArray<FeedItem> m_items;
};
