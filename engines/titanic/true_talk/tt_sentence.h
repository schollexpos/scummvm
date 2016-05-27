/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#ifndef TITANIC_TT_SENTENCE_H
#define TITANIC_TT_SENTENCE_H

#include "titanic/true_talk/tt_concept_node.h"
#include "titanic/true_talk/tt_npc_script.h"
#include "titanic/true_talk/tt_room_script.h"
#include "titanic/true_talk/tt_sentence_node.h"
#include "titanic/true_talk/tt_string.h"

namespace Titanic {

class CScriptHandler;
class TTword;

class TTsentenceConcept : public TTconceptNode {
public:
	TTsentenceConcept() : TTconceptNode() {}
	TTsentenceConcept(const TTsentenceConcept &src) : TTconceptNode(src) {}

	/**
	 * Adds a new sibling instance
	 */
	TTsentenceConcept *addSibling();
};

class TTsentence {
private:
	CScriptHandler *_owner;
	int _inputCtr;
	int _field34;
	int _field38;
	TTsentenceNode *_nodesP;
	int _field5C;
	int _status;
private:
	/**
	 * Copy sentence data from a given source
	 */
	void copyFrom(const TTsentence &src);
public:
	TTsentenceConcept _sentenceConcept;
	TTstring _initialLine;
	TTstring _normalizedLine;
	int _field58;
	TTroomScript *_roomScript;
	TTnpcScript *_npcScript;
	int _field2C;
public:
	TTsentence(int inputCtr, const TTstring &line, CScriptHandler *owner,
		TTroomScript *roomScript, TTnpcScript *npcScript);
	TTsentence(const TTsentence *src);
	~TTsentence();

	void set34(int v) { _field34 = v; }
	void set38(int v) { _field38 = v; }
	bool check2C() const { return _field2C > 1 && _field2C <= 10; }
	int concept18(TTconceptNode *conceptNode) {
		return conceptNode ? conceptNode->get18() : 0;
	}
	int get58() const { return _field58; }
	int is18(int val, const TTconceptNode *node) const;
	int is1C(int val, const TTconceptNode *node) const;

	int getStatus() const { return _status; }

	TTconcept *getFrameSlot(int conceptIndex, TTconceptNode *conceptNode);

	/**
	 * Adds a found vocab word to the list of words representing
	 * the player's input
	 * @param word		Word to node
	 */
	int storeVocabHit(TTword *word);

	bool fn2(int conceptIndex, const TTstring &str, TTconceptNode *conceptNode);
	bool fn4(int mode, int wordId, TTconceptNode *node);
};

} // End of namespace Titanic

#endif /* TITANIC_TT_SENTENCE_H */
