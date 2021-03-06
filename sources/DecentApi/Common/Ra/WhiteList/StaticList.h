#pragma once

#include "WhiteList.h"

namespace Decent
{
	namespace Ra
	{
		namespace WhiteList
		{
			class StaticList
			{
			public:
				StaticList() = delete;

				StaticList(const WhiteListType& whiteList);

				StaticList(WhiteListType&& whiteList);

				StaticList(const StaticList& rhs);

				StaticList(StaticList&& rhs);

				virtual ~StaticList();

				/**
				 * \brief	Gets the white list (map) as const reference.
				 *
				 * \return	The white list (map).
				 */
				const WhiteListType& GetMap() const;

				/**
				 * \brief	Check if the given hash is in the white list, and return the app name in the list.
				 *
				 * \param 		  	hashStr   	The hash in Base-64 string.
				 * \param [in,out]	outAppName	Name of the application.
				 *
				 * \return	True if the hash is in the white list, false if it's not.
				 */
				virtual bool CheckHash(const std::string& hashStr, std::string& outAppName) const;

				/**
				 * \brief	Check if the given hash is in the white list and the app name are match.
				 *
				 * \param	hashStr	The hash in Base-64 string.
				 * \param	appName	Name of the application.
				 *
				 * \return	True if the hash is in the white list and app name is the same, false if it's not.
				 */
				virtual bool CheckHashAndName(const std::string& hashStr, const std::string& appName) const;

				/**
				 * \brief	Check if 'this instance' is equivalent set of the 'right hand side'. Note: For each
				 * 			item, both hashes and app names are compared.
				 *
				 * \param	rhs	The right hand side.
				 *
				 * \return	True 'this' is equivalent set of the 'rhs', false if not.
				 */
				virtual bool IsEquivalentSetOf(const WhiteListType& rhs) const;

				/**
				* \brief	Check if 'this instance' is equivalent set of the 'right hand side'. Note: For each
				* 			item, both hashes and app names are compared.
				*
				* \param	rhs	The right hand side.
				*
				* \return	True 'this' is equivalent set of the 'rhs', false if not.
				*/
				virtual bool IsEquivalentSetOf(const StaticList& rhs) const { return this->IsEquivalentSetOf(rhs.m_listMap); }

				/**
				 * \brief	Check if 'this instance' is a subset of the 'right hand side'. Note: For each
				 * 			item, both hashes and app names are compared.
				 *
				 * \param	rhs	The right hand side.
				 *
				 * \return	True if 'this' is a subset of 'rhs', false if not.
				 */
				virtual bool IsSubsetOf(const WhiteListType& rhs) const;

				/**
				* \brief	Check Check if 'this instance' is a subset of the 'right hand side'. Note: For each
				* 			item, both hashes and app names are compared.
				*
				* \param	rhs	The right hand side.
				*
				* \return	True if 'this' is a subset of 'rhs', false if not.
				*/
				virtual bool IsSubsetOf(const StaticList& rhs) const { return this->IsSubsetOf(rhs.m_listMap); }

				/**
				 * \brief	Equality operator
				 * 			Check if the list in the right hand side is exact same as the left side.
				 * 			Basically, it is calling IsEquivalentSetOf.
				 *
				 * \param	rhs	The right hand side.
				 *
				 * \return	True if the parameters are considered equivalent.
				 */
				virtual bool operator==(const StaticList& rhs) const;

				/**
				 * \brief	Inequality operator. Basically, it is calling !operator==.
				 *
				 * \param	rhs	The right hand side.
				 *
				 * \return	True if the parameters are not considered equivalent.
				 */
				virtual bool operator!=(const StaticList& rhs) const;

				/**
				 * \brief	Greater-than-or-equal comparison operator. Check if 'right hand side' is a subset of the 'this instance'. Also see IsSubsetOf().
				 *
				 * \param	rhs	The right hand side.
				 *
				 * \return	True if 'rhs' is a subset of 'this', false if not.
				 */
				virtual bool operator>=(const StaticList& rhs) const;

				/**
				 * \brief	Less-than-or-equal comparison operator. Check if 'this instance' is a subset of the 'right hand side'. Also see IsSubsetOf().
				 *
				 * \param	rhs	The right hand side.
				 *
				 * \return	True if 'this' is a subset of 'rhs', false if not.
				 */
				virtual bool operator<=(const StaticList& rhs) const;

				/**
				 * \brief	Converts this white list to a string in JSON format.
				 *
				 * \return	a string in JSON format
				 */
				virtual std::string ToJsonString() const;

			private:
				WhiteListType m_listMap;
			};
		}
	}
}
