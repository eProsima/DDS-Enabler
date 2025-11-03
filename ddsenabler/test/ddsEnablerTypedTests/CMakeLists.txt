# Copyright 2024 Proyectos y Sistemas de Mantenimiento SL (eProsima).
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

set(TEST_NAME DDSEnablerTypedTest)

file(
    GLOB_RECURSE TEST_SOURCES
    DdsEnablerTypedTest.cpp
    types/*.cxx
)

if(WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj")
endif()

file(
    GLOB_RECURSE TEST_LIBRARY_SOURCES
    "${PROJECT_SOURCE_DIR}/src/cpp/*.c*"
)

list(FILTER TEST_LIBRARY_SOURCES EXCLUDE REGEX "/main.cpp")

set(TEST_LIST
    ddsenabler_send_samples_AliasAlias
    ddsenabler_send_samples_AliasArray
    ddsenabler_send_samples_AliasBitmask
    ddsenabler_send_samples_AliasBitset
    ddsenabler_send_samples_AliasBool
    ddsenabler_send_samples_AliasChar16
    ddsenabler_send_samples_AliasChar8
    ddsenabler_send_samples_AliasEnum
    ddsenabler_send_samples_AliasFloat128
    ddsenabler_send_samples_AliasFloat32
    ddsenabler_send_samples_AliasFloat64
    ddsenabler_send_samples_AliasInt16
    ddsenabler_send_samples_AliasInt32
    ddsenabler_send_samples_AliasInt64
    ddsenabler_send_samples_AliasMap
    ddsenabler_send_samples_AliasMultiArray
    ddsenabler_send_samples_AliasOctet
    ddsenabler_send_samples_AliasSequence
    ddsenabler_send_samples_AliasString16
    ddsenabler_send_samples_AliasString8
    ddsenabler_send_samples_AliasStruct
    ddsenabler_send_samples_AliasUInt32
    ddsenabler_send_samples_AliasUInt64
    ddsenabler_send_samples_AliasUint16
    ddsenabler_send_samples_AliasUnion
    ddsenabler_send_samples_BasicAnnotationsStruct
    ddsenabler_send_samples_EmptyAnnotatedStruct
    ddsenabler_send_samples_AppendableBooleanStruct
    ddsenabler_send_samples_AppendableCharStruct
    ddsenabler_send_samples_AppendableDoubleStruct
    ddsenabler_send_samples_AppendableEmptyInheritanceStruct
    ddsenabler_send_samples_AppendableEmptyStruct
    ddsenabler_send_samples_AppendableExtensibilityInheritance
    ddsenabler_send_samples_AppendableFloatStruct
    ddsenabler_send_samples_AppendableInheritanceEmptyStruct
    ddsenabler_send_samples_AppendableInheritanceStruct
    ddsenabler_send_samples_AppendableLongDoubleStruct
    ddsenabler_send_samples_AppendableLongLongStruct
    ddsenabler_send_samples_AppendableLongStruct
    ddsenabler_send_samples_AppendableOctetStruct
    ddsenabler_send_samples_AppendableShortStruct
    ddsenabler_send_samples_AppendableULongLongStruct
    ddsenabler_send_samples_AppendableULongStruct
    ddsenabler_send_samples_AppendableUShortStruct
    ddsenabler_send_samples_AppendableUnionStruct
    ddsenabler_send_samples_AppendableWCharStruct
    ddsenabler_send_samples_ArrayAlias
    ddsenabler_send_samples_ArrayBitMask
    ddsenabler_send_samples_ArrayBitset
    ddsenabler_send_samples_ArrayBoolean
    ddsenabler_send_samples_ArrayBoundedString
    ddsenabler_send_samples_ArrayBoundedWString
    ddsenabler_send_samples_ArrayChar
    ddsenabler_send_samples_ArrayDouble
    ddsenabler_send_samples_ArrayEnum
    ddsenabler_send_samples_ArrayFloat
    ddsenabler_send_samples_ArrayLong
    ddsenabler_send_samples_ArrayLongDouble
    ddsenabler_send_samples_ArrayLongLong
    ddsenabler_send_samples_ArrayMap
    ddsenabler_send_samples_ArrayMultiDimensionAlias
    ddsenabler_send_samples_ArrayMultiDimensionBitMask
    ddsenabler_send_samples_ArrayMultiDimensionBitset
    ddsenabler_send_samples_ArrayMultiDimensionBoolean
    ddsenabler_send_samples_ArrayMultiDimensionBoundedString
    ddsenabler_send_samples_ArrayMultiDimensionBoundedWString
    ddsenabler_send_samples_ArrayMultiDimensionChar
    ddsenabler_send_samples_ArrayMultiDimensionDouble
    ddsenabler_send_samples_ArrayMultiDimensionEnum
    ddsenabler_send_samples_ArrayMultiDimensionFloat
    ddsenabler_send_samples_ArrayMultiDimensionLiteralsAlias
    ddsenabler_send_samples_ArrayMultiDimensionLiteralsBitMask
    ddsenabler_send_samples_ArrayMultiDimensionLiteralsBitSet
    ddsenabler_send_samples_ArrayMultiDimensionLiteralsBoolean
    ddsenabler_send_samples_ArrayMultiDimensionLiteralsBoundedString
    ddsenabler_send_samples_ArrayMultiDimensionLiteralsBoundedWString
    ddsenabler_send_samples_ArrayMultiDimensionLiteralsChar
    ddsenabler_send_samples_ArrayMultiDimensionLiteralsDouble
    ddsenabler_send_samples_ArrayMultiDimensionLiteralsEnum
    ddsenabler_send_samples_ArrayMultiDimensionLiteralsFloat
    ddsenabler_send_samples_ArrayMultiDimensionLiteralsLong
    ddsenabler_send_samples_ArrayMultiDimensionLiteralsLongDouble
    ddsenabler_send_samples_ArrayMultiDimensionLiteralsLongLong
    ddsenabler_send_samples_ArrayMultiDimensionLiteralsMap
    ddsenabler_send_samples_ArrayMultiDimensionLiteralsOctet
    ddsenabler_send_samples_ArrayMultiDimensionLiteralsSequence
    ddsenabler_send_samples_ArrayMultiDimensionLiteralsShort
    ddsenabler_send_samples_ArrayMultiDimensionLiteralsString
    ddsenabler_send_samples_ArrayMultiDimensionLiteralsStructure
    ddsenabler_send_samples_ArrayMultiDimensionLiteralsULong
    ddsenabler_send_samples_ArrayMultiDimensionLiteralsULongLong
    ddsenabler_send_samples_ArrayMultiDimensionLiteralsUShort
    ddsenabler_send_samples_ArrayMultiDimensionLiteralsUnion
    ddsenabler_send_samples_ArrayMultiDimensionLiteralsWChar
    ddsenabler_send_samples_ArrayMultiDimensionLiteralsWString
    ddsenabler_send_samples_ArrayMultiDimensionLong
    ddsenabler_send_samples_ArrayMultiDimensionLongDouble
    ddsenabler_send_samples_ArrayMultiDimensionLongLong
    ddsenabler_send_samples_ArrayMultiDimensionMap
    ddsenabler_send_samples_ArrayMultiDimensionOctet
    ddsenabler_send_samples_ArrayMultiDimensionSequence
    ddsenabler_send_samples_ArrayMultiDimensionShort
    ddsenabler_send_samples_ArrayMultiDimensionString
    ddsenabler_send_samples_ArrayMultiDimensionStructure
    ddsenabler_send_samples_ArrayMultiDimensionULong
    ddsenabler_send_samples_ArrayMultiDimensionULongLong
    ddsenabler_send_samples_ArrayMultiDimensionUShort
    ddsenabler_send_samples_ArrayMultiDimensionUnion
    ddsenabler_send_samples_ArrayMultiDimensionWChar
    ddsenabler_send_samples_ArrayMultiDimensionWString
    ddsenabler_send_samples_ArrayOctet
    ddsenabler_send_samples_ArraySequence
    ddsenabler_send_samples_ArrayShort
    ddsenabler_send_samples_ArrayShortArray
    ddsenabler_send_samples_ArraySingleDimensionLiteralsAlias
    ddsenabler_send_samples_ArraySingleDimensionLiteralsBitMask
    ddsenabler_send_samples_ArraySingleDimensionLiteralsBitset
    ddsenabler_send_samples_ArraySingleDimensionLiteralsBoolean
    ddsenabler_send_samples_ArraySingleDimensionLiteralsBoundedString
    ddsenabler_send_samples_ArraySingleDimensionLiteralsBoundedWString
    ddsenabler_send_samples_ArraySingleDimensionLiteralsChar
    ddsenabler_send_samples_ArraySingleDimensionLiteralsDouble
    ddsenabler_send_samples_ArraySingleDimensionLiteralsEnum
    ddsenabler_send_samples_ArraySingleDimensionLiteralsFloat
    ddsenabler_send_samples_ArraySingleDimensionLiteralsLong
    ddsenabler_send_samples_ArraySingleDimensionLiteralsLongDouble
    ddsenabler_send_samples_ArraySingleDimensionLiteralsLongLong
    ddsenabler_send_samples_ArraySingleDimensionLiteralsMap
    ddsenabler_send_samples_ArraySingleDimensionLiteralsOctet
    ddsenabler_send_samples_ArraySingleDimensionLiteralsSequence
    ddsenabler_send_samples_ArraySingleDimensionLiteralsShort
    ddsenabler_send_samples_ArraySingleDimensionLiteralsShortArray
    ddsenabler_send_samples_ArraySingleDimensionLiteralsString
    ddsenabler_send_samples_ArraySingleDimensionLiteralsStructure
    ddsenabler_send_samples_ArraySingleDimensionLiteralsUnion
    ddsenabler_send_samples_ArraySingleDimensionLiteralsUnsignedLong
    ddsenabler_send_samples_ArraySingleDimensionLiteralsUnsignedLongLong
    ddsenabler_send_samples_ArraySingleDimensionLiteralsUnsignedShort
    ddsenabler_send_samples_ArraySingleDimensionLiteralsWChar
    ddsenabler_send_samples_ArraySingleDimensionLiteralsWString
    ddsenabler_send_samples_ArrayString
    ddsenabler_send_samples_ArrayStructure
    ddsenabler_send_samples_ArrayUInt8
    ddsenabler_send_samples_ArrayULong
    ddsenabler_send_samples_ArrayULongLong
    ddsenabler_send_samples_ArrayUShort
    ddsenabler_send_samples_ArrayUnion
    ddsenabler_send_samples_ArrayWChar
    ddsenabler_send_samples_ArrayWString
    ddsenabler_send_samples_BoundedBigArrays
    ddsenabler_send_samples_BoundedSmallArrays
    ddsenabler_send_samples_BitsetStruct
    ddsenabler_send_samples_ConstsLiteralsStruct
    ddsenabler_send_samples_Module2ConstsLiteralsStruct
    ddsenabler_send_samples_ModuleConstsLiteralsStruct
    ddsenabler_send_samples_BitMaskStructure
    ddsenabler_send_samples_BoundedBitMaskStructure
    ddsenabler_send_samples_EnumStructure
    ddsenabler_send_samples_FinalBooleanStruct
    ddsenabler_send_samples_FinalCharStruct
    ddsenabler_send_samples_FinalDoubleStruct
    ddsenabler_send_samples_FinalEmptyInheritanceStruct
    ddsenabler_send_samples_FinalEmptyStruct
    ddsenabler_send_samples_FinalExtensibilityInheritance
    ddsenabler_send_samples_FinalFloatStruct
    ddsenabler_send_samples_FinalInheritanceStruct
    ddsenabler_send_samples_FinalLongDoubleStruct
    ddsenabler_send_samples_FinalLongLongStruct
    ddsenabler_send_samples_FinalLongStruct
    ddsenabler_send_samples_FinalOctetStruct
    ddsenabler_send_samples_FinalShortStruct
    ddsenabler_send_samples_FinalULongLongStruct
    ddsenabler_send_samples_FinalULongStruct
    ddsenabler_send_samples_FinalUShortStruct
    ddsenabler_send_samples_FinalUnionStruct
    ddsenabler_send_samples_FinalWCharStruct
    ddsenabler_send_samples_InheritanceEmptyStruct
    ddsenabler_send_samples_InnerEmptyStructureHelper
    ddsenabler_send_samples_InnerStructureHelper
    ddsenabler_send_samples_BitsetsChildInheritanceStruct
    ddsenabler_send_samples_InnerEmptyStructureHelperChild
    ddsenabler_send_samples_InnerStructureHelperChild
    ddsenabler_send_samples_InnerStructureHelperChildChild
    ddsenabler_send_samples_InnerStructureHelperEmptyChild
    ddsenabler_send_samples_InnerStructureHelperEmptyChildChild
    ddsenabler_send_samples_StructAliasInheritanceStruct
    ddsenabler_send_samples_StructuresInheritanceStruct
    ddsenabler_send_samples_InheritanceKeyedEmptyStruct
    ddsenabler_send_samples_KeyedAppendable
    ddsenabler_send_samples_KeyedBooleanStruct
    ddsenabler_send_samples_KeyedCharStruct
    ddsenabler_send_samples_KeyedDoubleStruct
    ddsenabler_send_samples_KeyedEmptyInheritanceStruct
    ddsenabler_send_samples_KeyedEmptyStruct
    ddsenabler_send_samples_KeyedFinal
    ddsenabler_send_samples_KeyedFloatStruct
    ddsenabler_send_samples_KeyedInheritanceStruct
    ddsenabler_send_samples_KeyedLongDoubleStruct
    ddsenabler_send_samples_KeyedLongLongStruct
    ddsenabler_send_samples_KeyedLongStruct
    ddsenabler_send_samples_KeyedMutable
    ddsenabler_send_samples_KeyedOctetStruct
    ddsenabler_send_samples_KeyedShortStruct
    ddsenabler_send_samples_KeyedULongLongStruct
    ddsenabler_send_samples_KeyedULongStruct
    ddsenabler_send_samples_KeyedUShortStruct
    ddsenabler_send_samples_KeyedWCharStruct
    ddsenabler_send_samples_BoundedLargeMap
    ddsenabler_send_samples_BoundedSmallMap
    ddsenabler_send_samples_MapInnerAliasBoundedStringHelperBoolean
    ddsenabler_send_samples_MapInnerAliasBoundedStringHelperChar
    ddsenabler_send_samples_MapInnerAliasBoundedStringHelperDouble
    ddsenabler_send_samples_MapInnerAliasBoundedStringHelperFloat
    ddsenabler_send_samples_MapInnerAliasBoundedStringHelperInnerAliasArrayHelper
    ddsenabler_send_samples_MapInnerAliasBoundedStringHelperInnerAliasBoundedStringHelper
    ddsenabler_send_samples_MapInnerAliasBoundedStringHelperInnerAliasBoundedWStringHelper
    ddsenabler_send_samples_MapInnerAliasBoundedStringHelperInnerAliasHelper
    ddsenabler_send_samples_MapInnerAliasBoundedStringHelperInnerAliasMapHelper
    ddsenabler_send_samples_MapInnerAliasBoundedStringHelperInnerAliasSequenceHelper
    ddsenabler_send_samples_MapInnerAliasBoundedStringHelperInnerBitMaskHelper
    ddsenabler_send_samples_MapInnerAliasBoundedStringHelperInnerBitsetHelper
    ddsenabler_send_samples_MapInnerAliasBoundedStringHelperInnerEnumHelper
    ddsenabler_send_samples_MapInnerAliasBoundedStringHelperInnerStructureHelper
    ddsenabler_send_samples_MapInnerAliasBoundedStringHelperInnerUnionHelper
    ddsenabler_send_samples_MapInnerAliasBoundedStringHelperLong
    ddsenabler_send_samples_MapInnerAliasBoundedStringHelperLongDouble
    ddsenabler_send_samples_MapInnerAliasBoundedStringHelperLongLong
    ddsenabler_send_samples_MapInnerAliasBoundedStringHelperOctet
    ddsenabler_send_samples_MapInnerAliasBoundedStringHelperShort
    ddsenabler_send_samples_MapInnerAliasBoundedStringHelperString
    ddsenabler_send_samples_MapInnerAliasBoundedStringHelperULong
    ddsenabler_send_samples_MapInnerAliasBoundedStringHelperULongLong
    ddsenabler_send_samples_MapInnerAliasBoundedStringHelperUShort
    ddsenabler_send_samples_MapInnerAliasBoundedStringHelperWChar
    ddsenabler_send_samples_MapInnerAliasBoundedStringHelperWString
    ddsenabler_send_samples_MapKeyULongLongValueDouble
    ddsenabler_send_samples_MapKeyULongValueLongDouble
    ddsenabler_send_samples_MapKeyULongValueLongLong
    ddsenabler_send_samples_MapLongBoolean
    ddsenabler_send_samples_MapLongChar
    ddsenabler_send_samples_MapLongDouble
    ddsenabler_send_samples_MapLongFloat
    ddsenabler_send_samples_MapLongInnerAliasArrayHelper
    ddsenabler_send_samples_MapLongInnerAliasBoundedStringHelper
    ddsenabler_send_samples_MapLongInnerAliasBoundedWStringHelper
    ddsenabler_send_samples_MapLongInnerAliasHelper
    ddsenabler_send_samples_MapLongInnerAliasMapHelper
    ddsenabler_send_samples_MapLongInnerAliasSequenceHelper
    ddsenabler_send_samples_MapLongInnerBitMaskHelper
    ddsenabler_send_samples_MapLongInnerBitsetHelper
    ddsenabler_send_samples_MapLongInnerEnumHelper
    ddsenabler_send_samples_MapLongInnerStructureHelper
    ddsenabler_send_samples_MapLongInnerUnionHelper
    ddsenabler_send_samples_MapLongKeyLongDoubleValue
    ddsenabler_send_samples_MapLongKeyLongLongValue
    ddsenabler_send_samples_MapLongLong
    ddsenabler_send_samples_MapLongLongBoolean
    ddsenabler_send_samples_MapLongLongChar
    ddsenabler_send_samples_MapLongLongFloat
    ddsenabler_send_samples_MapLongLongInnerAliasArrayHelper
    ddsenabler_send_samples_MapLongLongInnerAliasBoundedStringHelper
    ddsenabler_send_samples_MapLongLongInnerAliasBoundedWStringHelper
    ddsenabler_send_samples_MapLongLongInnerAliasHelper
    ddsenabler_send_samples_MapLongLongInnerAliasMapHelper
    ddsenabler_send_samples_MapLongLongInnerAliasSequenceHelper
    ddsenabler_send_samples_MapLongLongInnerBitMaskHelper
    ddsenabler_send_samples_MapLongLongInnerBitsetHelper
    ddsenabler_send_samples_MapLongLongInnerEnumHelper
    ddsenabler_send_samples_MapLongLongInnerStructureHelper
    ddsenabler_send_samples_MapLongLongInnerUnionHelper
    ddsenabler_send_samples_MapLongLongKeyDoubleValue
    ddsenabler_send_samples_MapLongLongKeyLongValue
    ddsenabler_send_samples_MapLongLongLongDouble
    ddsenabler_send_samples_MapLongLongLongLong
    ddsenabler_send_samples_MapLongLongOctet
    ddsenabler_send_samples_MapLongLongShort
    ddsenabler_send_samples_MapLongLongString
    ddsenabler_send_samples_MapLongLongULong
    ddsenabler_send_samples_MapLongLongULongLong
    ddsenabler_send_samples_MapLongLongUShort
    ddsenabler_send_samples_MapLongLongWChar
    ddsenabler_send_samples_MapLongLongWString
    ddsenabler_send_samples_MapLongOctet
    ddsenabler_send_samples_MapLongShort
    ddsenabler_send_samples_MapLongString
    ddsenabler_send_samples_MapLongULong
    ddsenabler_send_samples_MapLongULongLong
    ddsenabler_send_samples_MapLongUShort
    ddsenabler_send_samples_MapLongWChar
    ddsenabler_send_samples_MapLongWString
    ddsenabler_send_samples_MapShortBoolean
    ddsenabler_send_samples_MapShortChar
    ddsenabler_send_samples_MapShortDouble
    ddsenabler_send_samples_MapShortFloat
    ddsenabler_send_samples_MapShortInnerAliasArrayHelper
    ddsenabler_send_samples_MapShortInnerAliasBoundedStringHelper
    ddsenabler_send_samples_MapShortInnerAliasBoundedWStringHelper
    ddsenabler_send_samples_MapShortInnerAliasHelper
    ddsenabler_send_samples_MapShortInnerAliasMapHelper
    ddsenabler_send_samples_MapShortInnerAliasSequenceHelper
    ddsenabler_send_samples_MapShortInnerBitMaskHelper
    ddsenabler_send_samples_MapShortInnerBitsetHelper
    ddsenabler_send_samples_MapShortInnerEnumHelper
    ddsenabler_send_samples_MapShortInnerStructureHelper
    ddsenabler_send_samples_MapShortInnerUnionHelper
    ddsenabler_send_samples_MapShortLong
    ddsenabler_send_samples_MapShortLongDouble
    ddsenabler_send_samples_MapShortLongLong
    ddsenabler_send_samples_MapShortOctet
    ddsenabler_send_samples_MapShortShort
    ddsenabler_send_samples_MapShortString
    ddsenabler_send_samples_MapShortULong
    ddsenabler_send_samples_MapShortULongLong
    ddsenabler_send_samples_MapShortUShort
    ddsenabler_send_samples_MapShortWChar
    ddsenabler_send_samples_MapShortWString
    ddsenabler_send_samples_MapStringBoolean
    ddsenabler_send_samples_MapStringChar
    ddsenabler_send_samples_MapStringDouble
    ddsenabler_send_samples_MapStringFloat
    ddsenabler_send_samples_MapStringInnerAliasArrayHelper
    ddsenabler_send_samples_MapStringInnerAliasBoundedStringHelper
    ddsenabler_send_samples_MapStringInnerAliasBoundedWStringHelper
    ddsenabler_send_samples_MapStringInnerAliasHelper
    ddsenabler_send_samples_MapStringInnerAliasMapHelper
    ddsenabler_send_samples_MapStringInnerAliasSequenceHelper
    ddsenabler_send_samples_MapStringInnerBitMaskHelper
    ddsenabler_send_samples_MapStringInnerBitsetHelper
    ddsenabler_send_samples_MapStringInnerEnumHelper
    ddsenabler_send_samples_MapStringInnerStructureHelper
    ddsenabler_send_samples_MapStringInnerUnionHelper
    ddsenabler_send_samples_MapStringLong
    ddsenabler_send_samples_MapStringLongDouble
    ddsenabler_send_samples_MapStringLongLong
    ddsenabler_send_samples_MapStringOctet
    ddsenabler_send_samples_MapStringShort
    ddsenabler_send_samples_MapStringString
    ddsenabler_send_samples_MapStringULong
    ddsenabler_send_samples_MapStringULongLong
    ddsenabler_send_samples_MapStringUShort
    ddsenabler_send_samples_MapStringWChar
    ddsenabler_send_samples_MapStringWString
    ddsenabler_send_samples_MapULongBoolean
    ddsenabler_send_samples_MapULongChar
    ddsenabler_send_samples_MapULongDouble
    ddsenabler_send_samples_MapULongFloat
    ddsenabler_send_samples_MapULongInnerAliasArrayHelper
    ddsenabler_send_samples_MapULongInnerAliasBoundedStringHelper
    ddsenabler_send_samples_MapULongInnerAliasBoundedWStringHelper
    ddsenabler_send_samples_MapULongInnerAliasHelper
    ddsenabler_send_samples_MapULongInnerAliasMapHelper
    ddsenabler_send_samples_MapULongInnerAliasSequenceHelper
    ddsenabler_send_samples_MapULongInnerBitMaskHelper
    ddsenabler_send_samples_MapULongInnerBitsetHelper
    ddsenabler_send_samples_MapULongInnerEnumHelper
    ddsenabler_send_samples_MapULongInnerStructureHelper
    ddsenabler_send_samples_MapULongInnerUnionHelper
    ddsenabler_send_samples_MapULongLong
    ddsenabler_send_samples_MapULongLongBoolean
    ddsenabler_send_samples_MapULongLongChar
    ddsenabler_send_samples_MapULongLongFloat
    ddsenabler_send_samples_MapULongLongInnerAliasArrayHelper
    ddsenabler_send_samples_MapULongLongInnerAliasBoundedStringHelper
    ddsenabler_send_samples_MapULongLongInnerAliasBoundedWStringHelper
    ddsenabler_send_samples_MapULongLongInnerAliasHelper
    ddsenabler_send_samples_MapULongLongInnerAliasMapHelper
    ddsenabler_send_samples_MapULongLongInnerAliasSequenceHelper
    ddsenabler_send_samples_MapULongLongInnerBitMaskHelper
    ddsenabler_send_samples_MapULongLongInnerBitsetHelper
    ddsenabler_send_samples_MapULongLongInnerEnumHelper
    ddsenabler_send_samples_MapULongLongInnerStructureHelper
    ddsenabler_send_samples_MapULongLongInnerUnionHelper
    ddsenabler_send_samples_MapULongLongLong
    ddsenabler_send_samples_MapULongLongLongDouble
    ddsenabler_send_samples_MapULongLongLongLong
    ddsenabler_send_samples_MapULongLongOctet
    ddsenabler_send_samples_MapULongLongShort
    ddsenabler_send_samples_MapULongLongString
    ddsenabler_send_samples_MapULongLongULong
    ddsenabler_send_samples_MapULongLongULongLong
    ddsenabler_send_samples_MapULongLongUShort
    ddsenabler_send_samples_MapULongLongWChar
    ddsenabler_send_samples_MapULongLongWString
    ddsenabler_send_samples_MapULongOctet
    ddsenabler_send_samples_MapULongShort
    ddsenabler_send_samples_MapULongString
    ddsenabler_send_samples_MapULongULong
    ddsenabler_send_samples_MapULongULongLong
    ddsenabler_send_samples_MapULongUShort
    ddsenabler_send_samples_MapULongWChar
    ddsenabler_send_samples_MapULongWString
    ddsenabler_send_samples_MapUShortBoolean
    ddsenabler_send_samples_MapUShortChar
    ddsenabler_send_samples_MapUShortDouble
    ddsenabler_send_samples_MapUShortFloat
    ddsenabler_send_samples_MapUShortInnerAliasArrayHelper
    ddsenabler_send_samples_MapUShortInnerAliasBoundedStringHelper
    ddsenabler_send_samples_MapUShortInnerAliasBoundedWStringHelper
    ddsenabler_send_samples_MapUShortInnerAliasHelper
    ddsenabler_send_samples_MapUShortInnerAliasMapHelper
    ddsenabler_send_samples_MapUShortInnerAliasSequenceHelper
    ddsenabler_send_samples_MapUShortInnerBitMaskHelper
    ddsenabler_send_samples_MapUShortInnerBitsetHelper
    ddsenabler_send_samples_MapUShortInnerEnumHelper
    ddsenabler_send_samples_MapUShortInnerStructureHelper
    ddsenabler_send_samples_MapUShortInnerUnionHelper
    ddsenabler_send_samples_MapUShortLong
    ddsenabler_send_samples_MapUShortLongDouble
    ddsenabler_send_samples_MapUShortLongLong
    ddsenabler_send_samples_MapUShortOctet
    ddsenabler_send_samples_MapUShortShort
    ddsenabler_send_samples_MapUShortString
    ddsenabler_send_samples_MapUShortULong
    ddsenabler_send_samples_MapUShortULongLong
    ddsenabler_send_samples_MapUShortUShort
    ddsenabler_send_samples_MapUShortWChar
    ddsenabler_send_samples_MapUShortWString
    ddsenabler_send_samples_MutableBooleanStruct
    ddsenabler_send_samples_MutableCharStruct
    ddsenabler_send_samples_MutableDoubleStruct
    ddsenabler_send_samples_MutableEmptyInheritanceStruct
    ddsenabler_send_samples_MutableEmptyStruct
    ddsenabler_send_samples_MutableExtensibilityInheritance
    ddsenabler_send_samples_MutableFloatStruct
    ddsenabler_send_samples_MutableInheritanceEmptyStruct
    ddsenabler_send_samples_MutableInheritanceStruct
    ddsenabler_send_samples_MutableLongDoubleStruct
    ddsenabler_send_samples_MutableLongLongStruct
    ddsenabler_send_samples_MutableLongStruct
    ddsenabler_send_samples_MutableOctetStruct
    ddsenabler_send_samples_MutableShortStruct
    ddsenabler_send_samples_MutableULongLongStruct
    ddsenabler_send_samples_MutableULongStruct
    ddsenabler_send_samples_MutableUShortStruct
    ddsenabler_send_samples_MutableUnionStruct
    ddsenabler_send_samples_MutableWCharStruct
    ddsenabler_send_samples_InnerStructOptional
    # TODO: comment out when optionals are supported
    # ddsenabler_send_samples_array_short_align_1_optional
    # ddsenabler_send_samples_array_short_align_2_optional
    # ddsenabler_send_samples_array_short_align_4_optional
    # ddsenabler_send_samples_array_short_optional
    # ddsenabler_send_samples_boolean_align_1_optional
    # ddsenabler_send_samples_boolean_align_2_optional
    # ddsenabler_send_samples_boolean_align_4_optional
    # ddsenabler_send_samples_boolean_optional
    # ddsenabler_send_samples_char_align_1_optional
    # ddsenabler_send_samples_char_align_2_optional
    # ddsenabler_send_samples_char_align_4_optional
    # ddsenabler_send_samples_char_optional
    # ddsenabler_send_samples_double_align_1_optional
    # ddsenabler_send_samples_double_align_2_optional
    # ddsenabler_send_samples_double_align_4_optional
    # ddsenabler_send_samples_double_optional
    # ddsenabler_send_samples_float_align_1_optional
    # ddsenabler_send_samples_float_align_2_optional
    # ddsenabler_send_samples_float_align_4_optional
    # ddsenabler_send_samples_float_optional
    # ddsenabler_send_samples_long_align_1_optional
    # ddsenabler_send_samples_long_align_2_optional
    # ddsenabler_send_samples_long_align_4_optional
    # ddsenabler_send_samples_long_optional
    # ddsenabler_send_samples_longdouble_align_1_optional
    # ddsenabler_send_samples_longdouble_align_2_optional
    # ddsenabler_send_samples_longdouble_align_4_optional
    # ddsenabler_send_samples_longdouble_optional
    # ddsenabler_send_samples_longlong_align_1_optional
    # ddsenabler_send_samples_longlong_align_2_optional
    # ddsenabler_send_samples_longlong_align_4_optional
    # ddsenabler_send_samples_longlong_optional
    # ddsenabler_send_samples_map_short_align_1_optional
    # ddsenabler_send_samples_map_short_align_2_optional
    # ddsenabler_send_samples_map_short_align_4_optional
    # ddsenabler_send_samples_map_short_optional
    # ddsenabler_send_samples_octet_align_1_optional
    # ddsenabler_send_samples_octet_align_2_optional
    # ddsenabler_send_samples_octet_align_4_optional
    # ddsenabler_send_samples_octet_optional
    # ddsenabler_send_samples_opt_struct_align_1_optional
    # ddsenabler_send_samples_opt_struct_align_2_optional
    # ddsenabler_send_samples_opt_struct_align_4_optional
    # ddsenabler_send_samples_opt_struct_optional
    # ddsenabler_send_samples_sequence_short_align_1_optional
    # ddsenabler_send_samples_sequence_short_align_2_optional
    # ddsenabler_send_samples_sequence_short_align_4_optional
    # ddsenabler_send_samples_sequence_short_optional
    # ddsenabler_send_samples_short_align_1_optional
    # ddsenabler_send_samples_short_align_2_optional
    # ddsenabler_send_samples_short_align_4_optional
    # ddsenabler_send_samples_short_optional
    # ddsenabler_send_samples_string_bounded_align_1_optional
    # ddsenabler_send_samples_string_bounded_align_2_optional
    # ddsenabler_send_samples_string_bounded_align_4_optional
    # ddsenabler_send_samples_string_bounded_optional
    # ddsenabler_send_samples_string_unbounded_align_1_optional
    # ddsenabler_send_samples_string_unbounded_align_2_optional
    # ddsenabler_send_samples_string_unbounded_align_4_optional
    # ddsenabler_send_samples_string_unbounded_optional
    # ddsenabler_send_samples_struct_align_1_optional
    # ddsenabler_send_samples_struct_align_2_optional
    # ddsenabler_send_samples_struct_align_4_optional
    # ddsenabler_send_samples_struct_optional
    # ddsenabler_send_samples_ulong_align_1_optional
    # ddsenabler_send_samples_ulong_align_2_optional
    # ddsenabler_send_samples_ulong_align_4_optional
    # ddsenabler_send_samples_ulong_optional
    # ddsenabler_send_samples_ulonglong_align_1_optional
    # ddsenabler_send_samples_ulonglong_align_2_optional
    # ddsenabler_send_samples_ulonglong_align_4_optional
    # ddsenabler_send_samples_ulonglong_optional
    # ddsenabler_send_samples_ushort_align_1_optional
    # ddsenabler_send_samples_ushort_align_2_optional
    # ddsenabler_send_samples_ushort_align_4_optional
    # ddsenabler_send_samples_ushort_optional
    # ddsenabler_send_samples_wchar_align_1_optional
    # ddsenabler_send_samples_wchar_align_2_optional
    # ddsenabler_send_samples_wchar_align_4_optional
    # ddsenabler_send_samples_wchar_optional
    ddsenabler_send_samples_BooleanStruct
    ddsenabler_send_samples_CharStruct
    ddsenabler_send_samples_DoubleStruct
    ddsenabler_send_samples_FloatStruct
    ddsenabler_send_samples_Int16Struct
    ddsenabler_send_samples_Int32Struct
    ddsenabler_send_samples_Int64Struct
    ddsenabler_send_samples_Int8Struct
    ddsenabler_send_samples_LongDoubleStruct
    ddsenabler_send_samples_LongLongStruct
    ddsenabler_send_samples_LongStruct
    ddsenabler_send_samples_OctetStruct
    ddsenabler_send_samples_ShortStruct
    ddsenabler_send_samples_ULongLongStruct
    ddsenabler_send_samples_ULongStruct
    ddsenabler_send_samples_UShortStruct
    ddsenabler_send_samples_Uint16Struct
    ddsenabler_send_samples_Uint32Struct
    ddsenabler_send_samples_Uint64Struct
    ddsenabler_send_samples_Uint8Struct
    ddsenabler_send_samples_WCharStruct
    ddsenabler_send_samples_BoundedBigSequences
    ddsenabler_send_samples_BoundedSmallSequences
    ddsenabler_send_samples_SequenceAlias
    ddsenabler_send_samples_SequenceBitMask
    ddsenabler_send_samples_SequenceBitset
    ddsenabler_send_samples_SequenceBoolean
    ddsenabler_send_samples_SequenceChar
    ddsenabler_send_samples_SequenceDouble
    ddsenabler_send_samples_SequenceEnum
    ddsenabler_send_samples_SequenceFloat
    ddsenabler_send_samples_SequenceLong
    ddsenabler_send_samples_SequenceLongDouble
    ddsenabler_send_samples_SequenceLongLong
    ddsenabler_send_samples_SequenceMap
    ddsenabler_send_samples_SequenceOctet
    ddsenabler_send_samples_SequenceSequence
    ddsenabler_send_samples_SequenceShort
    ddsenabler_send_samples_SequenceShortArray
    ddsenabler_send_samples_SequenceString
    ddsenabler_send_samples_SequenceStringBounded
    ddsenabler_send_samples_SequenceStructure
    ddsenabler_send_samples_SequenceULong
    ddsenabler_send_samples_SequenceULongLong
    ddsenabler_send_samples_SequenceUShort
    ddsenabler_send_samples_SequenceUnion
    ddsenabler_send_samples_SequenceWChar
    ddsenabler_send_samples_SequenceWString
    ddsenabler_send_samples_SequenceWStringBounded
    ddsenabler_send_samples_LargeStringStruct
    ddsenabler_send_samples_LargeWStringStruct
    ddsenabler_send_samples_SmallStringStruct
    ddsenabler_send_samples_SmallWStringStruct
    ddsenabler_send_samples_StringStruct
    ddsenabler_send_samples_WStringStruct
    ddsenabler_send_samples_StructAlias
    ddsenabler_send_samples_StructBitMask
    ddsenabler_send_samples_StructBitset
    ddsenabler_send_samples_StructBoolean
    ddsenabler_send_samples_StructBoundedString
    ddsenabler_send_samples_StructBoundedWString
    ddsenabler_send_samples_StructChar16
    ddsenabler_send_samples_StructChar8
    ddsenabler_send_samples_StructDouble
    ddsenabler_send_samples_StructEmpty
    ddsenabler_send_samples_StructEnum
    ddsenabler_send_samples_StructFloat
    ddsenabler_send_samples_StructLong
    ddsenabler_send_samples_StructLongDouble
    ddsenabler_send_samples_StructLongLong
    ddsenabler_send_samples_StructMap
    ddsenabler_send_samples_StructOctet
    ddsenabler_send_samples_StructSequence
    ddsenabler_send_samples_StructShort
    ddsenabler_send_samples_StructShortArray
    ddsenabler_send_samples_StructString
    ddsenabler_send_samples_StructStructure
    ddsenabler_send_samples_StructUnion
    ddsenabler_send_samples_StructUnsignedLong
    ddsenabler_send_samples_StructUnsignedLongLong
    ddsenabler_send_samples_StructUnsignedShort
    ddsenabler_send_samples_StructWString
    ddsenabler_send_samples_Structures
    ddsenabler_send_samples_bar
    ddsenabler_send_samples_root
    ddsenabler_send_samples_root1
    ddsenabler_send_samples_root2
    ddsenabler_send_samples_UnionArray
    ddsenabler_send_samples_UnionBoolean
    ddsenabler_send_samples_UnionBoundedString
    ddsenabler_send_samples_UnionBoundedWString
    ddsenabler_send_samples_UnionChar
    ddsenabler_send_samples_UnionDiscriminatorAlias
    ddsenabler_send_samples_UnionDiscriminatorBoolean
    ddsenabler_send_samples_UnionDiscriminatorChar
    ddsenabler_send_samples_UnionDiscriminatorEnum
    ddsenabler_send_samples_UnionDiscriminatorEnumLabel
    ddsenabler_send_samples_UnionDiscriminatorLong
    ddsenabler_send_samples_UnionDiscriminatorLongLong
    ddsenabler_send_samples_UnionDiscriminatorOctet
    ddsenabler_send_samples_UnionDiscriminatorShort
    ddsenabler_send_samples_UnionDiscriminatorULong
    ddsenabler_send_samples_UnionDiscriminatorULongLong
    ddsenabler_send_samples_UnionDiscriminatorUShort
    ddsenabler_send_samples_UnionDiscriminatorWChar
    ddsenabler_send_samples_UnionDouble
    ddsenabler_send_samples_UnionFloat
    ddsenabler_send_samples_UnionInnerAliasHelper
    ddsenabler_send_samples_UnionInnerBitMaskHelper
    ddsenabler_send_samples_UnionInnerBitsetHelper
    ddsenabler_send_samples_UnionInnerEnumHelper
    ddsenabler_send_samples_UnionInnerStructureHelper
    ddsenabler_send_samples_UnionInnerUnionHelper
    ddsenabler_send_samples_UnionLong
    ddsenabler_send_samples_UnionLongDouble
    ddsenabler_send_samples_UnionLongLong
    ddsenabler_send_samples_UnionMap
    ddsenabler_send_samples_UnionOctet
    ddsenabler_send_samples_UnionSequence
    ddsenabler_send_samples_UnionSeveralFields
    ddsenabler_send_samples_UnionSeveralFieldsWithDefault
    ddsenabler_send_samples_UnionShort
    ddsenabler_send_samples_UnionString
    ddsenabler_send_samples_UnionULong
    ddsenabler_send_samples_UnionULongLong
    ddsenabler_send_samples_UnionUShort
    ddsenabler_send_samples_UnionWChar
    ddsenabler_send_samples_UnionWString
)

set(TEST_NEEDED_SOURCES
)

set(TEST_EXTRA_HEADERS
    ${PROJECT_SOURCE_DIR}/test
    ${PROJECT_SOURCE_DIR}/test/ddsEnablerTypedTests/types
    ${PROJECT_SOURCE_DIR}/examples/utils
)

add_blackbox_executable(
    "${TEST_NAME}"
    "${TEST_SOURCES}"
    "${TEST_LIST}"
    "${TEST_NEEDED_SOURCES}"
    "${TEST_EXTRA_HEADERS}"
    "${TEST_LIBRARY_SOURCES}"
)

target_sources(blackbox_DDSEnablerTypedTest PRIVATE ${PROJECT_SOURCE_DIR}/examples/utils/Utils.cpp)
target_include_directories(blackbox_DDSEnablerTypedTest PRIVATE ${PROJECT_SOURCE_DIR}/examples/utils)

