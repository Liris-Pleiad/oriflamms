/* Copyright 2014 INSA-Lyon, IRHT, ZHAO Xiaojuan
 *
 * file: OriEntityManager.h
 * \author ZHAO Xiaojuan, Yann LEYDIER
 */
#ifndef ENTITY_MANAGER_H
#define ENTITY_MANAGER_H

#include <CRNStringUTF8.h>
#include <CRNUtils/CRNXml.h>
#include <map>

namespace ori
{
	/*! \brief XML entity to Unicode conversion table
	 */
	class EntityManager
	{
		public:
			/*! \brief Adds the content of a file to the table */
			static void AddEntityFile(const crn::Path &file_path, bool update = false);
			/*! \brief Adds an entity to the table */
			static bool AddEntity(const crn::StringUTF8 &key,const crn::StringUTF8 &value, bool update = false);
			/*! \brief Converts XML hexadecimal entities to Unicode */
			static crn::StringUTF8 ExpandUnicodeEntities(const crn::StringUTF8& xmltext);
			/*! \brief Loads an XML file and converts entities to Unicode */
			static std::unique_ptr<crn::xml::Document> ExpandXML(const crn::Path &fname);
			static void Save();
			/*! \brief Undoes modifications and reloads the last saved table */
			static void Reload();
			static void RemoveEntity(const crn::StringUTF8 &key);
			static const std::map<crn::StringUTF8, crn::StringUTF8>& GetMap() { return getInstance().entities; }

		private:
			static EntityManager& getInstance();
			static std::map<crn::StringUTF8, crn::StringUTF8>& getMap() { return getInstance().entities; }
			void load(const crn::Path &fname);

			EntityManager();
			std::map<crn::StringUTF8, crn::StringUTF8> entities;

	};
}
#endif

