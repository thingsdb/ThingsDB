package grammar

// This grammar is generated using the Grammar.export_go() method and
// should be used with the goleri module.
//
// Source class: RqlLang
// Created at: 2017-09-29 22:49:23

import (
	"regexp"

	"github.com/transceptor-technology/goleri"
)

// Element identifiers
const (
	NoGid = iota
	GidCompareOpr = iota
	GidElemId = iota
	GidFilter = iota
	GidKind = iota
	GidMark = iota
	GidProp = iota
	GidRFloat = iota
	GidRInt = iota
	GidRName = iota
	GidRStr = iota
	GidRUint = iota
	GidSTART = iota
	GidSelector = iota
	GidTAnd = iota
	GidTOr = iota
)

// RqlLang returns a compiled goleri grammar.
func RqlLang() *goleri.Grammar {
	rUint := goleri.NewRegex(GidRUint, regexp.MustCompile(`^[0-9]+`))
	rInt := goleri.NewRegex(GidRInt, regexp.MustCompile(`^[-+]?[0-9]+`))
	rFloat := goleri.NewRegex(GidRFloat, regexp.MustCompile(`^[-+]?[0-9]*\.?[0-9]+`))
	rStr := goleri.NewRegex(GidRStr, regexp.MustCompile(`^(?:"(?:[^"]*)")+`))
	rName := goleri.NewRegex(GidRName, regexp.MustCompile(`^[A-Za-z][A-Za-z0-9_]*`))
	elemId := goleri.NewRepeat(GidElemId, rUint, 1, 1)
	kind := goleri.NewRepeat(GidKind, rName, 1, 1)
	prop := goleri.NewRepeat(GidProp, rName, 1, 1)
	mark := goleri.NewRepeat(GidMark, rName, 1, 1)
	compareOpr := goleri.NewTokens(GidCompareOpr, "== != <= >= !~ < > ~")
	tAnd := goleri.NewToken(GidTAnd, "&&")
	tOr := goleri.NewToken(GidTOr, "||")
	filter := goleri.NewSequence(
		GidFilter,
		goleri.NewToken(NoGid, "{"),
		goleri.NewOptional(NoGid, goleri.NewPrio(
			NoGid,
			goleri.NewSequence(
				NoGid,
				prop,
				compareOpr,
				goleri.NewChoice(
					NoGid,
					false,
					rUint,
					rInt,
					rFloat,
					rStr,
				),
			),
			goleri.NewSequence(
				NoGid,
				goleri.NewToken(NoGid, "("),
				goleri.THIS,
				goleri.NewToken(NoGid, ")"),
			),
			goleri.NewSequence(
				NoGid,
				goleri.THIS,
				tAnd,
				goleri.THIS,
			),
			goleri.NewSequence(
				NoGid,
				goleri.THIS,
				tOr,
				goleri.THIS,
			),
		)),
		goleri.NewToken(NoGid, "}"),
	)
	selector := goleri.NewSequence(
		GidSelector,
		goleri.NewOptional(NoGid, goleri.NewToken(NoGid, "!")),
		goleri.NewChoice(
			NoGid,
			false,
			elemId,
			kind,
		),
		goleri.NewOptional(NoGid, goleri.NewChoice(
			NoGid,
			false,
			goleri.NewToken(NoGid, "*"),
			goleri.NewSequence(
				NoGid,
				goleri.NewToken(NoGid, "("),
				goleri.NewList(NoGid, goleri.NewSequence(
					NoGid,
					prop,
					goleri.NewOptional(NoGid, goleri.NewChoice(
						NoGid,
						false,
						goleri.NewSequence(
							NoGid,
							goleri.NewToken(NoGid, "#"),
							prop,
						),
						goleri.NewSequence(
							NoGid,
							goleri.NewToken(NoGid, "."),
							mark,
						),
					)),
					goleri.NewOptional(NoGid, filter),
				), goleri.NewToken(NoGid, ","), 1, 0, false),
				goleri.NewToken(NoGid, ")"),
			),
		)),
	)
	START := goleri.NewList(GidSTART, selector, goleri.NewToken(NoGid, ","), 1, 0, false)
	return goleri.NewGrammar(START, regexp.MustCompile(`^\w+`))
}
