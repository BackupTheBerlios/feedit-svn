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

	CFeedParser(const char* url) : m_feedType(FPFT_UNKNOWN)
	{
		CComPtr<MSXML2::IXMLDOMDocument2> xmldocument;
		xmldocument.CoCreateInstance(CComBSTR("Msxml2.DOMDocument"));
		ATLASSERT(xmldocument != NULL);
		xmldocument->async = FALSE;
		xmldocument->setProperty(_bstr_t("SelectionLanguage"), _variant_t("XPath"));
		xmldocument->setProperty(_bstr_t("SelectionNamespaces"), _variant_t("xmlns:rss09=\"http://my.netscape.com/rdf/simple/0.9/\" xmlns:rss10=\"http://purl.org/rss/1.0/\" xmlns:rdf=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\" xmlns:dc=\"http://purl.org/dc/elements/1.1/\" xmlns:atom=\"http://purl.org/atom/ns#\""));
		xmldocument->load(_variant_t(url));
		CComPtr<MSXML2::IXMLDOMNode> node;

		if((node = xmldocument->selectSingleNode(_bstr_t("/rdf:RDF/rss09:channel"))) != NULL)
		{
			m_feedType = FPFT_RSS09;
			CComPtr<MSXML2::IXMLDOMNode> titlenode = node->selectSingleNode(_bstr_t("rss09:title"));

			if(titlenode != NULL)
				m_feedName = (LPTSTR)titlenode->text;
		}
		else if((node = xmldocument->selectSingleNode(_bstr_t("/rdf:RDF/rss10:channel"))) != NULL)
		{
			m_feedType = FPFT_RSS10;
			CComPtr<MSXML2::IXMLDOMNode> titlenode = node->selectSingleNode(_bstr_t("rss10:title"));

			if(titlenode != NULL)
				m_feedName = (LPTSTR)titlenode->text;
		}
		else if((node = xmldocument->selectSingleNode(_bstr_t("/rss/channel"))) != NULL)
		{
			m_feedType = FPFT_RSS20;
			CComPtr<MSXML2::IXMLDOMNode> titlenode = node->selectSingleNode(_bstr_t("title"));

			if(titlenode != NULL)
				m_feedName = (LPTSTR)titlenode->text;
		}
		else if((node = xmldocument->selectSingleNode(_bstr_t("/atom:feed"))) != NULL)
		{
			m_feedType = FPFT_ATOM;
			CComPtr<MSXML2::IXMLDOMNode> titlenode = node->selectSingleNode(_bstr_t("atom:title"));

			if(titlenode != NULL)
				m_feedName = (LPTSTR)titlenode->text;
		}

		switch(m_feedType)
		{
		case FPFT_RSS09:
			{
				CComPtr<MSXML2::IXMLDOMNodeList> nodes = xmldocument->selectNodes(_bstr_t("/rdf:RDF/rss09:item"));

				if(nodes != NULL && nodes->length > 0)
				{
					CComPtr<MSXML2::IXMLDOMNode> node;

					while((node = nodes->nextNode()) != NULL)
					{
						CComPtr<MSXML2::IXMLDOMNode> titlenode = node->selectSingleNode(_bstr_t("rss09:title"));
						CComPtr<MSXML2::IXMLDOMNode> urlnode = node->selectSingleNode(_bstr_t("rss09:link"));
						CComPtr<MSXML2::IXMLDOMNode> descriptionnode = node->selectSingleNode(_bstr_t("rss09:description"));
						CComPtr<MSXML2::IXMLDOMNode> datenode = node->selectSingleNode(_bstr_t("dc:date"));
						FeedItem item;

						if(titlenode != NULL)
							item.m_title = (LPTSTR)titlenode->text;

						if(urlnode != NULL)
							item.m_url = (LPTSTR)urlnode->text;

						if(descriptionnode != NULL)
							item.m_description = (LPTSTR)descriptionnode->text;

						if(datenode != NULL)
							item.m_date = (LPTSTR)datenode->text;

						m_feedItems.Add(item);
					}
				}

				break;
			}
		case FPFT_RSS10:
			{
				CComPtr<MSXML2::IXMLDOMNodeList> nodes = xmldocument->selectNodes(_bstr_t("/rdf:RDF/rss10:item"));

				if(nodes != NULL && nodes->length > 0)
				{
					CComPtr<MSXML2::IXMLDOMNode> node;

					while((node = nodes->nextNode()) != NULL)
					{
						CComPtr<MSXML2::IXMLDOMNode> titlenode = node->selectSingleNode(_bstr_t("rss10:title"));
						CComPtr<MSXML2::IXMLDOMNode> urlnode = node->selectSingleNode(_bstr_t("rss10:link"));
						CComPtr<MSXML2::IXMLDOMNode> descriptionnode = node->selectSingleNode(_bstr_t("rss10:description"));
						CComPtr<MSXML2::IXMLDOMNode> datenode = node->selectSingleNode(_bstr_t("dc:date"));
						FeedItem item;

						if(titlenode != NULL)
							item.m_title = (LPTSTR)titlenode->text;

						if(urlnode != NULL)
							item.m_url = (LPTSTR)urlnode->text;

						if(descriptionnode != NULL)
							item.m_description = (LPTSTR)descriptionnode->text;

						if(datenode != NULL)
							item.m_date = (LPTSTR)datenode->text;

						m_feedItems.Add(item);
					}
				}

				break;
			}
		case FPFT_RSS20:
			{
				CComPtr<MSXML2::IXMLDOMNodeList> nodes = xmldocument->selectNodes(_bstr_t("/rss/channel/item"));

				if(nodes != NULL && nodes->length > 0)
				{
					CComPtr<MSXML2::IXMLDOMNode> node;

					while((node = nodes->nextNode()) != NULL)
					{
						CComPtr<MSXML2::IXMLDOMNode> titlenode = node->selectSingleNode(_bstr_t("title"));
						CComPtr<MSXML2::IXMLDOMNode> urlnode = node->selectSingleNode(_bstr_t("link"));
						CComPtr<MSXML2::IXMLDOMNode> descriptionnode = node->selectSingleNode(_bstr_t("description"));
						CComPtr<MSXML2::IXMLDOMNode> datenode = node->selectSingleNode(_bstr_t("date"));

						if(datenode == NULL)
							datenode = node->selectSingleNode(_bstr_t("pubDate"));

						FeedItem item;

						if(titlenode != NULL)
							item.m_title = (LPTSTR)titlenode->text;

						if(urlnode != NULL)
							item.m_url = (LPTSTR)urlnode->text;

						if(descriptionnode != NULL)
							item.m_description = (LPTSTR)descriptionnode->text;

						if(datenode != NULL)
							item.m_date = (LPTSTR)datenode->text;

						m_feedItems.Add(item);
					}
				}

				break;
			}
		case FPFT_ATOM:
			{
				CComPtr<MSXML2::IXMLDOMNodeList> nodes = xmldocument->selectNodes(_bstr_t("/atom:feed/atom:entry"));

				if(nodes != NULL && nodes->length > 0)
				{
					CComPtr<MSXML2::IXMLDOMNode> baseurlnode = xmldocument->selectSingleNode(_bstr_t("/atom:feed/atom:link[@rel=\"alternate\"]/@href"));
					CComPtr<MSXML2::IXMLDOMNode> node;

					while((node = nodes->nextNode()) != NULL)
					{
						CComPtr<MSXML2::IXMLDOMNode> titlenode = node->selectSingleNode(_bstr_t("atom:title"));
						CComPtr<MSXML2::IXMLDOMNode> urlnode = node->selectSingleNode(_bstr_t("atom:link[@rel=\"alternate\"]/@href"));
						CComPtr<MSXML2::IXMLDOMNode> descriptionnode = node->selectSingleNode(_bstr_t("atom:content"));
						CComPtr<MSXML2::IXMLDOMNode> datenode = node->selectSingleNode(_bstr_t("atom:issued"));
						FeedItem item;

						if(titlenode != NULL)
							item.m_title = (LPTSTR)titlenode->text;

						if(urlnode != NULL)
						{
							TCHAR buf[1024];
							DWORD buflen = 1024;
							AtlCombineUrl(baseurlnode->text, urlnode->text, buf, &buflen, ATL_URL_NO_ENCODE);
							item.m_url = buf;
						}

						if(descriptionnode != NULL)
							item.m_description = (LPTSTR)descriptionnode->text;

						if(datenode != NULL)
							item.m_date = (LPTSTR)datenode->text;

						m_feedItems.Add(item);
					}
				}

				break;
			}
		}
	}

	FeedType m_feedType;
	CAtlString m_feedName;
	CAtlArray<FeedItem> m_feedItems;
};
