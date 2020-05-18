//===--- TypeCheckRequests.h - Type Checking Requests -----------*- C++ -*-===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2017 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//
//
//  This file defines type checking requests.
//
//===----------------------------------------------------------------------===//
#ifndef SWIFT_TYPE_CHECK_REQUESTS_H
#define SWIFT_TYPE_CHECK_REQUESTS_H

#include "swift/AST/AnyFunctionRef.h"
#include "swift/AST/ASTTypeIDs.h"
#include "swift/AST/GenericSignature.h"
#include "swift/AST/Type.h"
#include "swift/AST/Evaluator.h"
#include "swift/AST/Pattern.h"
#include "swift/AST/SimpleRequest.h"
#include "swift/AST/SourceFile.h"
#include "swift/AST/TypeResolutionStage.h"
#include "swift/Basic/AnyValue.h"
#include "swift/Basic/Statistic.h"
#include "llvm/ADT/Hashing.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/TinyPtrVector.h"

namespace swift {

class AbstractStorageDecl;
class AccessorDecl;
enum class AccessorKind;
class ContextualPattern;
class DefaultArgumentExpr;
class ClosureExpr;
class GenericParamList;
class PrecedenceGroupDecl;
struct PropertyWrapperBackingPropertyInfo;
struct PropertyWrapperLValueness;
struct PropertyWrapperMutability;
class RequirementRepr;
class SpecializeAttr;
class TrailingWhereClause;
class TypeAliasDecl;
class TypeLoc;
class Witness;
class TypeResolution;
struct TypeWitnessAndDecl;
class ValueDecl;
enum class OpaqueReadOwnership: uint8_t;
class StorageImplInfo;

/// Display a nominal type or extension thereof.
void simple_display(
       llvm::raw_ostream &out,
       const llvm::PointerUnion<TypeDecl *, ExtensionDecl *> &value);

/// Request the type from the ith entry in the inheritance clause for the
/// given declaration.
class InheritedTypeRequest :
    public SimpleRequest<InheritedTypeRequest,
                         Type(llvm::PointerUnion<TypeDecl *, ExtensionDecl *>,
                              unsigned,
                              TypeResolutionStage),
                         RequestFlags::SeparatelyCached>
{
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  Type
  evaluate(Evaluator &evaluator,
           llvm::PointerUnion<TypeDecl *, ExtensionDecl *> decl,
           unsigned index,
           TypeResolutionStage stage) const;

public:
  // Source location
  SourceLoc getNearestLoc() const;

  // Caching
  bool isCached() const;
  Optional<Type> getCachedResult() const;
  void cacheResult(Type value) const;
};

/// Request the superclass type for the given class.
class SuperclassTypeRequest
    : public SimpleRequest<
          SuperclassTypeRequest, Type(NominalTypeDecl *, TypeResolutionStage),
          RequestFlags::SeparatelyCached | RequestFlags::DependencySink |
              RequestFlags::DependencySource> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  Type
  evaluate(Evaluator &evaluator, NominalTypeDecl *classDecl,
           TypeResolutionStage stage) const;

public:
  // Cycle handling
  void diagnoseCycle(DiagnosticEngine &diags) const;

public:
  // Separate caching.
  bool isCached() const;
  Optional<Type> getCachedResult() const;
  void cacheResult(Type value) const;

public:
  // Incremental dependencies
  evaluator::DependencySource
  readDependencySource(const evaluator::DependencyCollector &e) const;
  void writeDependencySink(evaluator::DependencyCollector &tracker,
                           Type t) const;
};

/// Request the raw type of the given enum.
class EnumRawTypeRequest :
    public SimpleRequest<EnumRawTypeRequest,
                         Type(EnumDecl *, TypeResolutionStage),
                         RequestFlags::SeparatelyCached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  Type
  evaluate(Evaluator &evaluator, EnumDecl *enumDecl,
           TypeResolutionStage stage) const;

public:
  // Cycle handling
  void diagnoseCycle(DiagnosticEngine &diags) const;

  // Separate caching.
  bool isCached() const;
  Optional<Type> getCachedResult() const;
  void cacheResult(Type value) const;
};

/// Request to determine the set of declarations that were are overridden
/// by the given declaration.
class OverriddenDeclsRequest :
  public SimpleRequest<OverriddenDeclsRequest,
                       llvm::TinyPtrVector<ValueDecl *>(ValueDecl *),
                       RequestFlags::SeparatelyCached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  llvm::TinyPtrVector<ValueDecl *>
  evaluate(Evaluator &evaluator, ValueDecl *decl) const;

public:
  // Separate caching.
  bool isCached() const { return true; }
  Optional<llvm::TinyPtrVector<ValueDecl *>> getCachedResult() const;
  void cacheResult(llvm::TinyPtrVector<ValueDecl *> value) const;
};

/// Determine whether the given declaration is exposed to Objective-C.
class IsObjCRequest :
    public SimpleRequest<IsObjCRequest,
                         bool(ValueDecl *),
                         RequestFlags::SeparatelyCached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  bool evaluate(Evaluator &evaluator, ValueDecl *decl) const;

public:
  // Separate caching.
  bool isCached() const { return true; }
  Optional<bool> getCachedResult() const;
  void cacheResult(bool value) const;
};

void simple_display(llvm::raw_ostream &out, CtorInitializerKind initKind);

/// Computes the kind of initializer for a given \c ConstructorDecl
class InitKindRequest :
    public SimpleRequest<InitKindRequest,
                         CtorInitializerKind(ConstructorDecl *),
                         RequestFlags::Cached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  CtorInitializerKind
  evaluate(Evaluator &evaluator, ConstructorDecl *decl) const;

public:
  // Caching.
  bool isCached() const { return true; }
};

/// Determine whether the given protocol declaration is class-bounded.
class ProtocolRequiresClassRequest :
    public SimpleRequest<ProtocolRequiresClassRequest,
                         bool(ProtocolDecl *),
                         RequestFlags::SeparatelyCached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  bool evaluate(Evaluator &evaluator, ProtocolDecl *decl) const;

public:
  // Cycle handling.
  void diagnoseCycle(DiagnosticEngine &diags) const;
  void noteCycleStep(DiagnosticEngine &diags) const;

  // Separate caching.
  bool isCached() const { return true; }
  Optional<bool> getCachedResult() const;
  void cacheResult(bool value) const;
};

/// Determine whether an existential conforming to a protocol can be matched
/// with a generic type parameter constrained to that protocol.
class ExistentialConformsToSelfRequest :
    public SimpleRequest<ExistentialConformsToSelfRequest,
                         bool(ProtocolDecl *),
                         RequestFlags::SeparatelyCached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  bool evaluate(Evaluator &evaluator, ProtocolDecl *decl) const;

public:
  // Cycle handling.
  void diagnoseCycle(DiagnosticEngine &diags) const;
  void noteCycleStep(DiagnosticEngine &diags) const;

  // Separate caching.
  bool isCached() const { return true; }
  Optional<bool> getCachedResult() const;
  void cacheResult(bool value) const;
};

/// Determine whether we are allowed to refer to an existential type conforming
/// to this protocol.
class ExistentialTypeSupportedRequest :
    public SimpleRequest<ExistentialTypeSupportedRequest,
                         bool(ProtocolDecl *),
                         RequestFlags::SeparatelyCached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  bool evaluate(Evaluator &evaluator, ProtocolDecl *decl) const;

public:
  // Cycle handling.
  void diagnoseCycle(DiagnosticEngine &diags) const;
  void noteCycleStep(DiagnosticEngine &diags) const;

  // Separate caching.
  bool isCached() const { return true; }
  Optional<bool> getCachedResult() const;
  void cacheResult(bool value) const;
};

/// Determine whether the given declaration is 'final'.
class IsFinalRequest :
    public SimpleRequest<IsFinalRequest,
                         bool(ValueDecl *),
                         RequestFlags::SeparatelyCached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  bool evaluate(Evaluator &evaluator, ValueDecl *decl) const;

public:
  // Separate caching.
  bool isCached() const { return true; }
  Optional<bool> getCachedResult() const;
  void cacheResult(bool value) const;
};

/// Determine whether the given declaration is 'dynamic''.
class IsDynamicRequest :
    public SimpleRequest<IsDynamicRequest,
                         bool(ValueDecl *),
                         RequestFlags::SeparatelyCached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  bool evaluate(Evaluator &evaluator, ValueDecl *decl) const;

public:
  // Separate caching.
  bool isCached() const { return true; }
  Optional<bool> getCachedResult() const;
  void cacheResult(bool value) const;
};

/// Compute the requirements that describe a protocol.
class RequirementSignatureRequest :
    public SimpleRequest<RequirementSignatureRequest,
                         ArrayRef<Requirement>(ProtocolDecl *),
                         RequestFlags::SeparatelyCached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  ArrayRef<Requirement>
  evaluate(Evaluator &evaluator, ProtocolDecl *proto) const;

public:
  // Separate caching.
  bool isCached() const { return true; }
  Optional<ArrayRef<Requirement>> getCachedResult() const;
  void cacheResult(ArrayRef<Requirement> value) const;
};

/// Compute the default definition type of an associated type.
class DefaultDefinitionTypeRequest :
    public SimpleRequest<DefaultDefinitionTypeRequest,
                         Type(AssociatedTypeDecl *),
                         RequestFlags::Cached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  Type evaluate(Evaluator &evaluator, AssociatedTypeDecl *decl) const;

public:
  // Caching.
  bool isCached() const { return true; }
};

/// Describes the owner of a where clause, from which we can extract
/// requirements.
struct WhereClauseOwner {
  /// The declaration context in which the where clause will be evaluated.
  DeclContext *dc;

  /// The source of the where clause, which can be a generic parameter list
  /// or a declaration that can have a where clause.
  llvm::PointerUnion<GenericParamList *, TrailingWhereClause *,
                     SpecializeAttr *, DifferentiableAttr *>
      source;

  WhereClauseOwner(GenericContext *genCtx);
  WhereClauseOwner(AssociatedTypeDecl *atd);

  WhereClauseOwner(DeclContext *dc, GenericParamList *genericParams)
      : dc(dc), source(genericParams) {}

  WhereClauseOwner(DeclContext *dc, SpecializeAttr *attr)
      : dc(dc), source(attr) {}

  WhereClauseOwner(DeclContext *dc, DifferentiableAttr *attr)
      : dc(dc), source(attr) {}

  SourceLoc getLoc() const;

  friend hash_code hash_value(const WhereClauseOwner &owner) {
    return llvm::hash_value(owner.source.getOpaqueValue());
  }

  friend bool operator==(const WhereClauseOwner &lhs,
                         const WhereClauseOwner &rhs) {
    return lhs.source.getOpaqueValue() == rhs.source.getOpaqueValue();
  }

  friend bool operator!=(const WhereClauseOwner &lhs,
                         const WhereClauseOwner &rhs) {
    return !(lhs == rhs);
  }

public:
  /// Retrieve the array of requirements.
  MutableArrayRef<RequirementRepr> getRequirements() const;

  /// Visit each of the requirements,
  ///
  /// \returns true after short-circuiting if the callback returned \c true
  /// for any of the requirements.
  bool
  visitRequirements(TypeResolutionStage stage,
                    llvm::function_ref<bool(Requirement, RequirementRepr *)>
                        callback) const &&;
};

void simple_display(llvm::raw_ostream &out, const WhereClauseOwner &owner);

/// Retrieve a requirement from the where clause of the given declaration.
class RequirementRequest :
    public SimpleRequest<RequirementRequest,
                         Requirement(WhereClauseOwner, unsigned,
                                     TypeResolutionStage),
                         RequestFlags::SeparatelyCached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  /// Retrieve the requirement this request operates on.
  RequirementRepr &getRequirement() const;

  // Evaluation.
  Requirement evaluate(Evaluator &evaluator,
                       WhereClauseOwner,
                       unsigned index,
                       TypeResolutionStage stage) const;

public:
  // Source location
  SourceLoc getNearestLoc() const;

  // Cycle handling.
  void noteCycleStep(DiagnosticEngine &diags) const;
                           
  // Separate caching.
  bool isCached() const;
  Optional<Requirement> getCachedResult() const;
  void cacheResult(Requirement value) const;
};

/// Generate the USR for the given declaration.
class USRGenerationRequest :
    public SimpleRequest<USRGenerationRequest,
                         std::string(const ValueDecl*),
                         RequestFlags::Cached>
{
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  std::string evaluate(Evaluator &eval, const ValueDecl *d) const;

public:
  // Caching
  bool isCached() const { return true; }
};

/// Generate the mangling for the given local type declaration.
class MangleLocalTypeDeclRequest :
    public SimpleRequest<MangleLocalTypeDeclRequest,
                         std::string(const TypeDecl*),
                         RequestFlags::Cached>
{
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  std::string evaluate(Evaluator &eval, const TypeDecl *d) const;

public:
  // Caching
  bool isCached() const { return true; }
};

void simple_display(llvm::raw_ostream &out, const KnownProtocolKind);

// Find the type in the cache or look it up
class DefaultTypeRequest
    : public SimpleRequest<DefaultTypeRequest,
                           Type(KnownProtocolKind, const DeclContext *),
                           RequestFlags::SeparatelyCached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  Type evaluate(Evaluator &eval, KnownProtocolKind, const DeclContext *) const;

public:
  // Caching
  bool isCached() const { return true; }
  Optional<Type> getCachedResult() const;
  void cacheResult(Type value) const;
};

/// Retrieve information about a property wrapper type.
class PropertyWrapperTypeInfoRequest
  : public SimpleRequest<PropertyWrapperTypeInfoRequest,
                         PropertyWrapperTypeInfo(NominalTypeDecl *),
                         RequestFlags::Cached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  PropertyWrapperTypeInfo
  evaluate(Evaluator &eval, NominalTypeDecl *nominal) const;

public:
  // Caching
  bool isCached() const;
};

/// Request the nominal type declaration to which the given custom attribute
/// refers.
class AttachedPropertyWrappersRequest :
    public SimpleRequest<AttachedPropertyWrappersRequest,
                         llvm::TinyPtrVector<CustomAttr *>(VarDecl *),
                         RequestFlags::Cached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  llvm::TinyPtrVector<CustomAttr *>
  evaluate(Evaluator &evaluator, VarDecl *) const;

public:
  // Caching
  bool isCached() const;
};

/// Request the raw (possibly unbound generic) type of the property wrapper
/// that is attached to the given variable.
class AttachedPropertyWrapperTypeRequest :
    public SimpleRequest<AttachedPropertyWrapperTypeRequest,
                         Type(VarDecl *, unsigned),
                         RequestFlags::Cached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  Type
  evaluate(Evaluator &evaluator, VarDecl *var, unsigned i) const;

public:
  // Caching
  bool isCached() const;
};

/// Request the nominal type declaration to which the given custom attribute
/// refers.
class PropertyWrapperBackingPropertyTypeRequest :
    public SimpleRequest<PropertyWrapperBackingPropertyTypeRequest,
                         Type(VarDecl *),
                         RequestFlags::Cached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  Type
  evaluate(Evaluator &evaluator, VarDecl *var) const;

public:
  // Caching
  bool isCached() const;
};

/// Request information about the mutability of composed property wrappers.
class PropertyWrapperMutabilityRequest :
    public SimpleRequest<PropertyWrapperMutabilityRequest,
                         Optional<PropertyWrapperMutability> (VarDecl *),
                         RequestFlags::Cached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  Optional<PropertyWrapperMutability>
  evaluate(Evaluator &evaluator, VarDecl *var) const;

public:
  // Caching
  bool isCached() const;
};

/// Request information about the l-valueness of composed property wrappers.
class PropertyWrapperLValuenessRequest :
    public SimpleRequest<PropertyWrapperLValuenessRequest,
                         Optional<PropertyWrapperLValueness> (VarDecl *),
                         RequestFlags::Cached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  Optional<PropertyWrapperLValueness>
  evaluate(Evaluator &evaluator, VarDecl *var) const;

public:
  // Caching
  bool isCached() const;
};

/// Request information about the backing property for properties that have
/// attached property wrappers.
class PropertyWrapperBackingPropertyInfoRequest :
    public SimpleRequest<PropertyWrapperBackingPropertyInfoRequest,
                         PropertyWrapperBackingPropertyInfo(VarDecl *),
                         RequestFlags::Cached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  PropertyWrapperBackingPropertyInfo
  evaluate(Evaluator &evaluator, VarDecl *var) const;

public:
  // Caching
  bool isCached() const;
};

/// Retrieve the structural type of an alias type.
class StructuralTypeRequest :
    public SimpleRequest<StructuralTypeRequest,
                         Type(TypeAliasDecl*),
                         RequestFlags::Cached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  Type evaluate(Evaluator &eval, TypeAliasDecl *d) const;

public:
  // Caching.
  bool isCached() const { return true; }
};

/// Request the fragile function kind for the context.
class FragileFunctionKindRequest :
    public SimpleRequest<FragileFunctionKindRequest,
                         FragileFunctionKind(DeclContext*),
                         RequestFlags::Cached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  FragileFunctionKind evaluate(Evaluator &eval, DeclContext *context) const;

public:
  // Caching.
  bool isCached() const { return true; }
};

void simple_display(llvm::raw_ostream &out, FragileFunctionKind value);

void simple_display(llvm::raw_ostream &out, ResilienceExpansion value);

/// Request the custom attribute which attaches a function builder to the
/// given declaration.
class AttachedFunctionBuilderRequest :
    public SimpleRequest<AttachedFunctionBuilderRequest,
                         CustomAttr *(ValueDecl *),
                         RequestFlags::Cached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  CustomAttr *
  evaluate(Evaluator &evaluator, ValueDecl *decl) const;

public:
  // Caching
  bool isCached() const;
};

/// Request the function builder type attached to the given declaration,
/// if any.
class FunctionBuilderTypeRequest :
    public SimpleRequest<FunctionBuilderTypeRequest,
                         Type(ValueDecl *),
                         RequestFlags::Cached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  Type
  evaluate(Evaluator &evaluator, ValueDecl *decl) const;

public:
  // Caching
  bool isCached() const { return true; }
};

/// Request a function's self access kind.
class SelfAccessKindRequest :
    public SimpleRequest<SelfAccessKindRequest,
                         SelfAccessKind(FuncDecl *),
                         RequestFlags::SeparatelyCached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  SelfAccessKind
  evaluate(Evaluator &evaluator, FuncDecl *func) const;

public:
  // Separate caching.
  bool isCached() const { return true; }
  Optional<SelfAccessKind> getCachedResult() const;
  void cacheResult(SelfAccessKind value) const;
};

/// Request whether the storage has a mutating getter.
class IsGetterMutatingRequest :
    public SimpleRequest<IsGetterMutatingRequest,
                         bool(AbstractStorageDecl *),
                         RequestFlags::SeparatelyCached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  bool
  evaluate(Evaluator &evaluator, AbstractStorageDecl *func) const;

public:
  // Separate caching.
  bool isCached() const { return true; }
  Optional<bool> getCachedResult() const;
  void cacheResult(bool value) const;
};

/// Request whether the storage has a mutating getter.
class IsSetterMutatingRequest :
    public SimpleRequest<IsSetterMutatingRequest,
                         bool(AbstractStorageDecl *),
                         RequestFlags::SeparatelyCached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  bool
  evaluate(Evaluator &evaluator, AbstractStorageDecl *func) const;

public:
  // Separate caching.
  bool isCached() const { return true; }
  Optional<bool> getCachedResult() const;
  void cacheResult(bool value) const;
};

/// Request whether reading the storage yields a borrowed value.
class OpaqueReadOwnershipRequest :
    public SimpleRequest<OpaqueReadOwnershipRequest,
                         OpaqueReadOwnership(AbstractStorageDecl *),
                         RequestFlags::SeparatelyCached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  OpaqueReadOwnership
  evaluate(Evaluator &evaluator, AbstractStorageDecl *storage) const;

public:
  // Separate caching.
  bool isCached() const { return true; }
  Optional<OpaqueReadOwnership> getCachedResult() const;
  void cacheResult(OpaqueReadOwnership value) const;
};

/// Request to build the underlying storage for a lazy property.
class LazyStoragePropertyRequest :
    public SimpleRequest<LazyStoragePropertyRequest,
                         VarDecl *(VarDecl *),
                         RequestFlags::Cached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  VarDecl *
  evaluate(Evaluator &evaluator, VarDecl *lazyVar) const;

public:
  bool isCached() const { return true; }
};

/// Request to type check the body of the given function up to the given
/// source location.
///
/// Produces true if an error occurred, false otherwise.
/// FIXME: it would be far better to return the type-checked body.
class TypeCheckFunctionBodyUntilRequest :
    public SimpleRequest<TypeCheckFunctionBodyUntilRequest,
                         bool(AbstractFunctionDecl *, SourceLoc),
                         RequestFlags::Cached|RequestFlags::DependencySource> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  bool
  evaluate(Evaluator &evaluator, AbstractFunctionDecl *func,
           SourceLoc endTypeCheckLoc) const;

public:
  bool isCached() const { return true; }

public:
  // Incremental dependencies.
  evaluator::DependencySource
  readDependencySource(const evaluator::DependencyCollector &) const;
};

/// Request to obtain a list of stored properties in a nominal type.
///
/// This will include backing storage for lazy properties and
/// property wrappers, synthesizing them if necessary.
class StoredPropertiesRequest :
    public SimpleRequest<StoredPropertiesRequest,
                         ArrayRef<VarDecl *>(NominalTypeDecl *),
                         RequestFlags::Cached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  ArrayRef<VarDecl *>
  evaluate(Evaluator &evaluator, NominalTypeDecl *decl) const;

public:
  bool isCached() const { return true; }
};

/// Request to obtain a list of stored properties in a nominal type,
/// together with any missing members corresponding to stored
/// properties that could not be deserialized.
///
/// This will include backing storage for lazy properties and
/// property wrappers, synthesizing them if necessary.
class StoredPropertiesAndMissingMembersRequest :
    public SimpleRequest<StoredPropertiesAndMissingMembersRequest,
                         ArrayRef<Decl *>(NominalTypeDecl *),
                         RequestFlags::Cached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  ArrayRef<Decl *>
  evaluate(Evaluator &evaluator, NominalTypeDecl *decl) const;

public:
  bool isCached() const { return true; }
};

class StorageImplInfoRequest :
    public SimpleRequest<StorageImplInfoRequest,
                         StorageImplInfo(AbstractStorageDecl *),
                         RequestFlags::SeparatelyCached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  StorageImplInfo
  evaluate(Evaluator &evaluator, AbstractStorageDecl *decl) const;

public:
  // Separate caching.
  bool isCached() const { return true; }
  Optional<StorageImplInfo> getCachedResult() const;
  void cacheResult(StorageImplInfo value) const;
};

class RequiresOpaqueAccessorsRequest :
    public SimpleRequest<RequiresOpaqueAccessorsRequest,
                         bool(VarDecl *),
                         RequestFlags::SeparatelyCached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  bool
  evaluate(Evaluator &evaluator, VarDecl *decl) const;

public:
  // Separate caching.
  bool isCached() const { return true; }
  Optional<bool> getCachedResult() const;
  void cacheResult(bool value) const;
};

class RequiresOpaqueModifyCoroutineRequest :
    public SimpleRequest<RequiresOpaqueModifyCoroutineRequest,
                         bool(AbstractStorageDecl *),
                         RequestFlags::SeparatelyCached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  bool
  evaluate(Evaluator &evaluator, AbstractStorageDecl *decl) const;

public:
  // Separate caching.
  bool isCached() const { return true; }
  Optional<bool> getCachedResult() const;
  void cacheResult(bool value) const;
};

class IsAccessorTransparentRequest :
    public SimpleRequest<IsAccessorTransparentRequest,
                         bool(AccessorDecl *),
                         RequestFlags::SeparatelyCached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  bool
  evaluate(Evaluator &evaluator, AccessorDecl *decl) const;

public:
  // Separate caching.
  bool isCached() const { return true; }
  Optional<bool> getCachedResult() const;
  void cacheResult(bool value) const;
};

class SynthesizeAccessorRequest :
    public SimpleRequest<SynthesizeAccessorRequest,
                         AccessorDecl *(AbstractStorageDecl *,
                                        AccessorKind),
                         RequestFlags::SeparatelyCached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  AccessorDecl *
  evaluate(Evaluator &evaluator, AbstractStorageDecl *decl,
           AccessorKind kind) const;

public:
  // Separate caching.
  bool isCached() const { return true; }
  Optional<AccessorDecl *> getCachedResult() const;
  void cacheResult(AccessorDecl *value) const;
};

class EmittedMembersRequest :
    public SimpleRequest<EmittedMembersRequest,
                         DeclRange(ClassDecl *),
                         RequestFlags::SeparatelyCached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  DeclRange
  evaluate(Evaluator &evaluator, ClassDecl *classDecl) const;

public:
  // Separate caching.
  bool isCached() const { return true; }
  Optional<DeclRange> getCachedResult() const;
  void cacheResult(DeclRange value) const;
};

class IsImplicitlyUnwrappedOptionalRequest :
    public SimpleRequest<IsImplicitlyUnwrappedOptionalRequest,
                         bool(ValueDecl *),
                         RequestFlags::SeparatelyCached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  bool
  evaluate(Evaluator &evaluator, ValueDecl *value) const;

public:
  // Separate caching.
  bool isCached() const { return true; }
  Optional<bool> getCachedResult() const;
  void cacheResult(bool value) const;
};

class ClassAncestryFlagsRequest :
    public SimpleRequest<ClassAncestryFlagsRequest,
                         AncestryFlags (ClassDecl *),
                         RequestFlags::Cached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  AncestryFlags
  evaluate(Evaluator &evaluator, ClassDecl *value) const;

public:
  // Caching.
  bool isCached() const { return true; }
};

void simple_display(llvm::raw_ostream &out, AncestryFlags value);

class AbstractGenericSignatureRequest :
    public SimpleRequest<AbstractGenericSignatureRequest,
                         GenericSignature (const GenericSignatureImpl *,
                                           SmallVector<GenericTypeParamType *, 2>,
                                           SmallVector<Requirement, 2>),
                         RequestFlags::Cached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  GenericSignature
  evaluate(Evaluator &evaluator,
           const GenericSignatureImpl *baseSignature,
           SmallVector<GenericTypeParamType *, 2> addedParameters,
           SmallVector<Requirement, 2> addedRequirements) const;

public:
  // Separate caching.
  bool isCached() const;

  /// Abstract generic signature requests never have source-location info.
  SourceLoc getNearestLoc() const {
    return SourceLoc();
  }
};

class InferredGenericSignatureRequest :
    public SimpleRequest<InferredGenericSignatureRequest,
                         GenericSignature (ModuleDecl *,
                                            const GenericSignatureImpl *,
                                            GenericParamSource,
                                            SmallVector<Requirement, 2>,
                                            SmallVector<TypeLoc, 2>,
                                            bool),
                         RequestFlags::Cached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  GenericSignature
  evaluate(Evaluator &evaluator,
           ModuleDecl *module,
           const GenericSignatureImpl *baseSignature,
           GenericParamSource paramSource,
           SmallVector<Requirement, 2> addedRequirements,
           SmallVector<TypeLoc, 2> inferenceSources,
           bool allowConcreteGenericParams) const;

public:
  // Separate caching.
  bool isCached() const;

  /// Inferred generic signature requests don't have source-location info.
  SourceLoc getNearestLoc() const {
    return SourceLoc();
  }
                           
  // Cycle handling.
  void noteCycleStep(DiagnosticEngine &diags) const;
};

void simple_display(llvm::raw_ostream &out, const TypeLoc source);

class ExtendedTypeRequest
    : public SimpleRequest<ExtendedTypeRequest,
                           Type(ExtensionDecl *),
                           RequestFlags::Cached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  Type evaluate(Evaluator &eval, ExtensionDecl *) const;
public:
  // Caching.
  bool isCached() const { return true; }
};

class FunctionOperatorRequest :
    public SimpleRequest<FunctionOperatorRequest,
                         OperatorDecl *(FuncDecl *),
                         RequestFlags::Cached> {
public:
  using SimpleRequest::SimpleRequest;
  
private:
  friend SimpleRequest;
  
  // Evaluation.
  OperatorDecl *
  evaluate(Evaluator &evaluator, FuncDecl *value) const;
  
public:
  // Caching.
  bool isCached() const { return true; }
};

class GenericSignatureRequest :
    public SimpleRequest<GenericSignatureRequest,
                         GenericSignature (GenericContext *),
                         RequestFlags::SeparatelyCached> {
public:
  using SimpleRequest::SimpleRequest;
  
private:
  friend SimpleRequest;
  
  // Evaluation.
  GenericSignature
  evaluate(Evaluator &evaluator, GenericContext *value) const;
  
public:
  // Separate caching.
  bool isCached() const { return true; }
  Optional<GenericSignature> getCachedResult() const;
  void cacheResult(GenericSignature value) const;
};

/// Compute the underlying interface type of a typealias.
class UnderlyingTypeRequest :
    public SimpleRequest<UnderlyingTypeRequest,
                         Type(TypeAliasDecl *),
                         RequestFlags::SeparatelyCached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  Type evaluate(Evaluator &evaluator, TypeAliasDecl *decl) const;

public:
  // Caching.
  bool isCached() const { return true; }
  Optional<Type> getCachedResult() const;
  void cacheResult(Type value) const;
  void diagnoseCycle(DiagnosticEngine &diags) const;
};

/// Looks up the precedence group of an operator declaration.
class OperatorPrecedenceGroupRequest
    : public SimpleRequest<OperatorPrecedenceGroupRequest,
                           PrecedenceGroupDecl *(InfixOperatorDecl *),
                           RequestFlags::Cached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  PrecedenceGroupDecl *
  evaluate(Evaluator &evaluator, InfixOperatorDecl *PGD) const;

public:
  // Separate caching.
  bool isCached() const { return true; }
};

/// Computes the raw values for an enum type.
class EnumRawValuesRequest :
    public SimpleRequest<EnumRawValuesRequest,
                         evaluator::SideEffect (EnumDecl *, TypeResolutionStage),
                         RequestFlags::SeparatelyCached> {
public:
  using SimpleRequest::SimpleRequest;
  
private:
  friend SimpleRequest;
  
  // Evaluation.
  evaluator::SideEffect
  evaluate(Evaluator &evaluator, EnumDecl *ED, TypeResolutionStage stage) const;
  
public:
  // Cycle handling.
  void diagnoseCycle(DiagnosticEngine &diags) const;
  void noteCycleStep(DiagnosticEngine &diags) const;
                           
  // Separate caching.
  bool isCached() const;
  Optional<evaluator::SideEffect> getCachedResult() const;
  void cacheResult(evaluator::SideEffect value) const;
};

/// Determines if an override is ABI compatible with its base method.
class IsABICompatibleOverrideRequest
    : public SimpleRequest<IsABICompatibleOverrideRequest, bool(ValueDecl *),
                           RequestFlags::Cached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  bool evaluate(Evaluator &evaluator, ValueDecl *decl) const;

public:
  // Caching.
  bool isCached() const { return true; }
};

/// Builds an opaque result type for a declaration.
class OpaqueResultTypeRequest
    : public SimpleRequest<OpaqueResultTypeRequest,
                           OpaqueTypeDecl *(ValueDecl *),
                           RequestFlags::Cached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  OpaqueTypeDecl *
  evaluate(Evaluator &evaluator, ValueDecl *VD) const;

public:
  // Caching.
  bool isCached() const { return true; }
};

/// Determines if a function declaration is 'static'.
class IsStaticRequest :
    public SimpleRequest<IsStaticRequest,
                         bool(FuncDecl *),
                         RequestFlags::SeparatelyCached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  bool
  evaluate(Evaluator &evaluator, FuncDecl *value) const;

public:
  // Separate caching.
  bool isCached() const { return true; }
  Optional<bool> getCachedResult() const;
  void cacheResult(bool value) const;
};

/// Determines if a method override should introduce a new vtable entry,
/// because the override is not ABI compatible, or the base method is
/// less visible than the override.
class NeedsNewVTableEntryRequest
    : public SimpleRequest<NeedsNewVTableEntryRequest,
                           bool(AbstractFunctionDecl *),
                           RequestFlags::SeparatelyCached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  bool evaluate(Evaluator &evaluator, AbstractFunctionDecl *decl) const;

public:
  // Separate caching.
  bool isCached() const { return true; }
  Optional<bool> getCachedResult() const;
  void cacheResult(bool value) const;
};

/// Determines the specifier for a parameter (inout, __owned, etc).
class ParamSpecifierRequest
    : public SimpleRequest<ParamSpecifierRequest,
                           ParamSpecifier(ParamDecl *),
                           RequestFlags::SeparatelyCached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  ParamSpecifier
  evaluate(Evaluator &evaluator, ParamDecl *decl) const;

public:
  // Separate caching.
  bool isCached() const { return true; }
  Optional<ParamSpecifier> getCachedResult() const;
  void cacheResult(ParamSpecifier value) const;
};

/// Determines the result type of a function or element type of a subscript.
class ResultTypeRequest
    : public SimpleRequest<ResultTypeRequest,
                           Type(ValueDecl *),
                           RequestFlags::SeparatelyCached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  TypeLoc &getResultTypeLoc() const;

  // Evaluation.
  Type evaluate(Evaluator &evaluator, ValueDecl *decl) const;

public:
  // Separate caching.
  bool isCached() const { return true; }
  Optional<Type> getCachedResult() const;
  void cacheResult(Type value) const;
};

class PatternBindingEntryRequest
    : public SimpleRequest<PatternBindingEntryRequest,
                           const PatternBindingEntry *(PatternBindingDecl *,
                                                       unsigned),
                           RequestFlags::SeparatelyCached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  const PatternBindingEntry *
  evaluate(Evaluator &evaluator, PatternBindingDecl *PBD, unsigned i) const;

public:
  // Separate caching.
  bool isCached() const { return true; }
  Optional<const PatternBindingEntry *> getCachedResult() const;
  void cacheResult(const PatternBindingEntry *value) const;
};

class NamingPatternRequest
    : public SimpleRequest<NamingPatternRequest, NamedPattern *(VarDecl *),
                           RequestFlags::SeparatelyCached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  NamedPattern * evaluate(Evaluator &evaluator, VarDecl *VD) const;

public:
  // Separate caching.
  bool isCached() const { return true; }
  Optional<NamedPattern *> getCachedResult() const;
  void cacheResult(NamedPattern *P) const;
};

class InterfaceTypeRequest :
    public SimpleRequest<InterfaceTypeRequest,
                         Type (ValueDecl *),
                         RequestFlags::SeparatelyCached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  Type
  evaluate(Evaluator &evaluator, ValueDecl *decl) const;

public:
  // Separate caching.
  bool isCached() const { return true; }
  Optional<Type> getCachedResult() const;
  void cacheResult(Type value) const;
};

struct PrecedenceGroupDescriptor {
  enum PathDirection : bool {
    LowerThan = false,
    HigherThan = true,
  };
  DeclContext *dc;
  Identifier ident;
  SourceLoc nameLoc;
  // Exists for diagnostics. Does not contribute to the descriptor otherwise.
  Optional<PathDirection> pathDirection;

  SourceLoc getLoc() const;

  friend llvm::hash_code hash_value(const PrecedenceGroupDescriptor &owner) {
    return llvm::hash_combine(owner.dc,
                              owner.ident.getAsOpaquePointer(),
                              owner.nameLoc.getOpaquePointerValue());
  }

  friend bool operator==(const PrecedenceGroupDescriptor &lhs,
                         const PrecedenceGroupDescriptor &rhs) {
    return lhs.dc == rhs.dc &&
           lhs.ident == rhs.ident &&
           lhs.nameLoc == rhs.nameLoc;
  }

  friend bool operator!=(const PrecedenceGroupDescriptor &lhs,
                         const PrecedenceGroupDescriptor &rhs) {
    return !(lhs == rhs);
  }
};

void simple_display(llvm::raw_ostream &out, const PrecedenceGroupDescriptor &d);

class ValidatePrecedenceGroupRequest
    : public SimpleRequest<ValidatePrecedenceGroupRequest,
                           TinyPtrVector<PrecedenceGroupDecl *>(
                               PrecedenceGroupDescriptor),
                           RequestFlags::Cached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  TinyPtrVector<PrecedenceGroupDecl *>
  evaluate(Evaluator &evaluator, PrecedenceGroupDescriptor descriptor) const;

public:
  // Cycle handling.
  void diagnoseCycle(DiagnosticEngine &diags) const;
  void noteCycleStep(DiagnosticEngine &diags) const;

  // Source location
  SourceLoc getNearestLoc() const;

  // Separate caching.
  bool isCached() const { return true; }
};

/// Computes whether all of the stored properties in a nominal type have initial
/// values.
class AreAllStoredPropertiesDefaultInitableRequest
    : public SimpleRequest<AreAllStoredPropertiesDefaultInitableRequest,
                           bool(NominalTypeDecl *), RequestFlags::Cached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  bool evaluate(Evaluator &evaluator, NominalTypeDecl *decl) const;

public:
  // Caching.
  bool isCached() const { return true; }
};

/// Computes whether this type has a user-defined designated initializer. This
/// does not include a synthesized designated initializer used to satisfy a
/// conformance.
class HasUserDefinedDesignatedInitRequest
    : public SimpleRequest<HasUserDefinedDesignatedInitRequest,
                           bool(NominalTypeDecl *), RequestFlags::Cached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  bool evaluate(Evaluator &evaluator, NominalTypeDecl *decl) const;

public:
  // Caching.
  bool isCached() const { return true; }
};

/// Checks whether this type has a synthesized memberwise initializer.
class HasMemberwiseInitRequest
    : public SimpleRequest<HasMemberwiseInitRequest, bool(StructDecl *),
                           RequestFlags::Cached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  bool evaluate(Evaluator &evaluator, StructDecl *decl) const;

public:
  // Caching.
  bool isCached() const { return true; }
};

/// Synthesizes a memberwise initializer for a given type.
class SynthesizeMemberwiseInitRequest
    : public SimpleRequest<SynthesizeMemberwiseInitRequest,
                           ConstructorDecl *(NominalTypeDecl *),
                           RequestFlags::Cached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  ConstructorDecl *evaluate(Evaluator &evaluator, NominalTypeDecl *decl) const;

public:
  // Caching.
  bool isCached() const { return true; }
};

/// Resolves the effective memberwise initializer for a given type.
///
/// An effective memberwise initializer is either a synthesized memberwise
/// initializer or a user-defined initializer with the same type.
///
/// See `NominalTypeDecl::getEffectiveMemberwiseInitializer` for details.
class ResolveEffectiveMemberwiseInitRequest
    : public SimpleRequest<ResolveEffectiveMemberwiseInitRequest,
                           ConstructorDecl *(NominalTypeDecl *),
                           RequestFlags::Cached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  ConstructorDecl *evaluate(Evaluator &evaluator, NominalTypeDecl *decl) const;

public:
  // Caching.
  bool isCached() const { return true; }
};

/// Checks whether this type has a synthesized zero parameter default
/// initializer.
class HasDefaultInitRequest
    : public SimpleRequest<HasDefaultInitRequest, bool(NominalTypeDecl *),
                           RequestFlags::Cached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  bool evaluate(Evaluator &evaluator,
                                NominalTypeDecl *decl) const;

public:
  // Caching.
  bool isCached() const { return true; }
};

/// Synthesizes a default initializer for a given type.
class SynthesizeDefaultInitRequest
    : public SimpleRequest<SynthesizeDefaultInitRequest,
                           ConstructorDecl *(NominalTypeDecl *),
                           RequestFlags::Cached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  ConstructorDecl * evaluate(Evaluator &evaluator,
                             NominalTypeDecl *decl) const;

public:
  // Caching.
  bool isCached() const { return true; }
};

class CompareDeclSpecializationRequest
    : public SimpleRequest<CompareDeclSpecializationRequest,
                           bool(DeclContext *, ValueDecl *, ValueDecl *, bool),
                           RequestFlags::Cached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  bool evaluate(Evaluator &evaluator, DeclContext *DC,
                ValueDecl *VD1, ValueDecl *VD2,
                bool dynamic) const;

public:
  // Caching.
  bool isCached() const { return true; }
};

/// Checks whether this declaration inherits its superclass' designated and
/// convenience initializers.
class InheritsSuperclassInitializersRequest
    : public SimpleRequest<InheritsSuperclassInitializersRequest,
                           bool(ClassDecl *), RequestFlags::SeparatelyCached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  bool evaluate(Evaluator &evaluator, ClassDecl *decl) const;

public:
  // Separate caching.
  bool isCached() const { return true; }
  Optional<bool> getCachedResult() const;
  void cacheResult(bool value) const;
};

// The actions this request takes are all huge layering violations.
//
// Please do not add any more.
enum class ImplicitMemberAction : uint8_t {
  ResolveImplicitInit,
  ResolveCodingKeys,
  ResolveEncodable,
  ResolveDecodable,
};

class ResolveImplicitMemberRequest
    : public SimpleRequest<ResolveImplicitMemberRequest,
                           evaluator::SideEffect(NominalTypeDecl *,
                                                 ImplicitMemberAction),
                           RequestFlags::Uncached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  evaluator::SideEffect
  evaluate(Evaluator &evaluator, NominalTypeDecl *NTD,
           ImplicitMemberAction action) const;

public:
  // Separate caching.
  bool isCached() const { return true; }
};

class TypeWitnessRequest
    : public SimpleRequest<TypeWitnessRequest,
                           TypeWitnessAndDecl(NormalProtocolConformance *,
                                              AssociatedTypeDecl *),
                           RequestFlags::SeparatelyCached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  TypeWitnessAndDecl
  evaluate(Evaluator &evaluator, NormalProtocolConformance *conformance,
           AssociatedTypeDecl *ATD) const;

public:
  // Separate caching.
  bool isCached() const { return true; }
  Optional<TypeWitnessAndDecl> getCachedResult() const;
  void cacheResult(TypeWitnessAndDecl value) const;
};

class ValueWitnessRequest
    : public SimpleRequest<ValueWitnessRequest,
                           Witness(NormalProtocolConformance *, ValueDecl *),
                           RequestFlags::SeparatelyCached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  Witness evaluate(Evaluator &evaluator,
                   NormalProtocolConformance *conformance,
                   ValueDecl *VD) const;

public:
  // Separate caching.
  bool isCached() const { return true; }
  Optional<Witness> getCachedResult() const;
  void cacheResult(Witness value) const;
};

enum class FunctionBuilderBodyPreCheck : uint8_t {
  /// There were no problems pre-checking the closure.
  Okay,

  /// There was an error pre-checking the closure.
  Error,

  /// The closure has a return statement.
  HasReturnStmt,
};

class PreCheckFunctionBuilderRequest
    : public SimpleRequest<PreCheckFunctionBuilderRequest,
                           FunctionBuilderBodyPreCheck(AnyFunctionRef),
                           RequestFlags::Cached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  FunctionBuilderBodyPreCheck
  evaluate(Evaluator &evaluator, AnyFunctionRef fn) const;

public:
  // Separate caching.
  bool isCached() const { return true; }
};

/// Computes whether a class has a circular reference in its inheritance
/// hierarchy.
class HasCircularInheritanceRequest
    : public SimpleRequest<HasCircularInheritanceRequest, bool(ClassDecl *),
                           RequestFlags::Cached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  bool evaluate(Evaluator &evaluator, ClassDecl *decl) const;

public:
  // Cycle handling.
  void diagnoseCycle(DiagnosticEngine &diags) const;
  void noteCycleStep(DiagnosticEngine &diags) const;

  // Cached.
  bool isCached() const { return true; }
};

/// Computes whether a protocol has a circular reference in its list of
/// inherited protocols.
class HasCircularInheritedProtocolsRequest
    : public SimpleRequest<HasCircularInheritedProtocolsRequest,
                           bool(ProtocolDecl *), RequestFlags::Cached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  bool evaluate(Evaluator &evaluator, ProtocolDecl *decl) const;

public:
  // Cycle handling.
  void diagnoseCycle(DiagnosticEngine &diags) const;
  void noteCycleStep(DiagnosticEngine &diags) const;

  // Cached.
  bool isCached() const { return true; }
};

/// Computes whether an enum's raw value has a circular reference.
class HasCircularRawValueRequest
    : public SimpleRequest<HasCircularRawValueRequest, bool(EnumDecl *),
                           RequestFlags::Cached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  bool evaluate(Evaluator &evaluator, EnumDecl *decl) const;

public:
  // Cycle handling.
  void diagnoseCycle(DiagnosticEngine &diags) const;
  void noteCycleStep(DiagnosticEngine &diags) const;

  // Cached.
  bool isCached() const { return true; }
};

/// Computes an initializer context for a parameter with a default argument.
class DefaultArgumentInitContextRequest
    : public SimpleRequest<DefaultArgumentInitContextRequest,
                           Initializer *(ParamDecl *),
                           RequestFlags::SeparatelyCached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  Initializer * evaluate(Evaluator &evaluator,
                                         ParamDecl *param) const;

public:
  // Separate caching.
  bool isCached() const { return true; }
  Optional<Initializer *> getCachedResult() const;
  void cacheResult(Initializer *init) const;
};

/// Computes the fully type-checked default argument expression for a given
/// parameter.
class DefaultArgumentExprRequest
    : public SimpleRequest<DefaultArgumentExprRequest, Expr *(ParamDecl *),
                           RequestFlags::SeparatelyCached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  Expr *evaluate(Evaluator &evaluator, ParamDecl *param) const;

public:
  // Separate caching.
  bool isCached() const { return true; }
  Optional<Expr *> getCachedResult() const;
  void cacheResult(Expr *expr) const;
};

/// Computes the fully type-checked caller-side default argument within the
/// context of the call site that it will be inserted into.
class CallerSideDefaultArgExprRequest
    : public SimpleRequest<CallerSideDefaultArgExprRequest,
                           Expr *(DefaultArgumentExpr *),
                           RequestFlags::SeparatelyCached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  Expr * evaluate(Evaluator &evaluator, DefaultArgumentExpr *defaultExpr) const;

public:
  // Separate caching.
  bool isCached() const { return true; }
  Optional<Expr *> getCachedResult() const;
  void cacheResult(Expr *expr) const;
};

/// Computes whether this is a type that supports being called through the
/// implementation of a \c callAsFunction method.
class IsCallableNominalTypeRequest
    : public SimpleRequest<IsCallableNominalTypeRequest,
                           bool(CanType, DeclContext *), RequestFlags::Cached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  bool evaluate(Evaluator &evaluator, CanType ty, DeclContext *dc) const;

public:
  // Cached.
  bool isCached() const { return true; }
};

class DynamicallyReplacedDeclRequest
    : public SimpleRequest<DynamicallyReplacedDeclRequest,
                           ValueDecl *(ValueDecl *), RequestFlags::Cached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  ValueDecl * evaluate(Evaluator &evaluator, ValueDecl *VD) const;

public:
  // Caching.
  bool isCached() const { return true; }
};

class TypeCheckSourceFileRequest
    : public SimpleRequest<
          TypeCheckSourceFileRequest, evaluator::SideEffect(SourceFile *),
          RequestFlags::SeparatelyCached | RequestFlags::DependencySource> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  evaluator::SideEffect
  evaluate(Evaluator &evaluator, SourceFile *SF) const;

public:
  // Separate caching.
  bool isCached() const { return true; }
  Optional<evaluator::SideEffect> getCachedResult() const;
  void cacheResult(evaluator::SideEffect) const;

public:
  // Incremental dependencies.
  evaluator::DependencySource
  readDependencySource(const evaluator::DependencyCollector &) const;
};

/// Computes whether the specified type or a super-class/super-protocol has the
/// @dynamicMemberLookup attribute on it.
class HasDynamicMemberLookupAttributeRequest
    : public SimpleRequest<HasDynamicMemberLookupAttributeRequest,
                           bool(CanType), RequestFlags::Cached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  bool evaluate(Evaluator &evaluator, CanType ty) const;

public:
  bool isCached() const {
    // Don't cache types containing type variables, as they must not outlive
    // the constraint system that created them.
    auto ty = std::get<0>(getStorage());
    return !ty->hasTypeVariable();
  }
};

/// Computes whether the specified type or a super-class/super-protocol has the
/// @dynamicCallable attribute on it.
class HasDynamicCallableAttributeRequest
    : public SimpleRequest<HasDynamicCallableAttributeRequest,
                           bool(CanType), RequestFlags::Cached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  bool evaluate(Evaluator &evaluator, CanType ty) const;

public:
  bool isCached() const {
    // Don't cache types containing type variables, as they must not outlive
    // the constraint system that created them.
    auto ty = std::get<0>(getStorage());
    return !ty->hasTypeVariable();
  }
};

/// Determines the type of a given pattern.
///
/// Note that this returns the "raw" pattern type, which can involve
/// unresolved types and unbound generic types where type inference is
/// allowed.
class PatternTypeRequest
    : public SimpleRequest<PatternTypeRequest, Type(ContextualPattern),
                           RequestFlags::Cached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  Type evaluate(Evaluator &evaluator, ContextualPattern pattern) const;

public:
  bool isCached() const { return true; }

  SourceLoc getNearestLoc() const {
    return std::get<0>(getStorage()).getPattern()->getLoc();
  }
};

/// List SPI group ids declared on a decl.
class SPIGroupsRequest :
    public SimpleRequest<SPIGroupsRequest,
                         llvm::ArrayRef<Identifier>(const Decl *),
                         RequestFlags::Cached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  llvm::ArrayRef<Identifier>
  evaluate(Evaluator &evaluator, const Decl *decl) const;

public:
  bool isCached() const { return true; }
};


/// Type-checks a `@differentiable` attribute and returns the resolved parameter
/// indices on success. On failure, emits diagnostics and returns `nullptr`.
///
/// Currently, this request resolves other `@differentiable` attribute
/// components but mutates them in place:
/// - `JVPFunction`
/// - `VJPFunction`
/// - `DerivativeGenericSignature`
class DifferentiableAttributeTypeCheckRequest
    : public SimpleRequest<DifferentiableAttributeTypeCheckRequest,
                           IndexSubset *(DifferentiableAttr *),
                           RequestFlags::SeparatelyCached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  IndexSubset * evaluate(Evaluator &evaluator,
                         DifferentiableAttr *attr) const;

public:
  // Separate caching.
  bool isCached() const { return true; }
  Optional<IndexSubset *> getCachedResult() const;
  void cacheResult(IndexSubset *value) const;
};

/// Resolves the referenced original declaration for a `@derivative` attribute.
class DerivativeAttrOriginalDeclRequest
    : public SimpleRequest<DerivativeAttrOriginalDeclRequest,
                           AbstractFunctionDecl *(DerivativeAttr *),
                           RequestFlags::Cached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  AbstractFunctionDecl *evaluate(Evaluator &evaluator,
                                 DerivativeAttr *attr) const;

public:
  // Caching.
  bool isCached() const { return true; }
};

/// Checks whether a type eraser has a viable initializer.
class TypeEraserHasViableInitRequest
    : public SimpleRequest<TypeEraserHasViableInitRequest,
                           bool(TypeEraserAttr *, ProtocolDecl *),
                           RequestFlags::Cached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation
  bool evaluate(Evaluator &evaluator, TypeEraserAttr *attr,
                ProtocolDecl *protocol) const;

public:
  bool isCached() const { return true; }
};

/// Looks up the decls that a scoped import references, ensuring the import is
/// valid.
///
/// A "scoped import" is an import which only covers one particular
/// declaration, such as:
///
///     import class Foundation.NSString
///
class ScopedImportLookupRequest
    : public SimpleRequest<ScopedImportLookupRequest,
                           ArrayRef<ValueDecl *>(ImportDecl *),
                           RequestFlags::Cached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  ArrayRef<ValueDecl *>
  evaluate(Evaluator &evaluator, ImportDecl *import) const;

public:
  // Cached.
  bool isCached() const { return true; }
};

/// Determine whether closure body has any explicit `return`
/// statements which could produce a non-void result.
class ClosureHasExplicitResultRequest
    : public SimpleRequest<ClosureHasExplicitResultRequest, bool(ClosureExpr *),
                           RequestFlags::Cached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  bool evaluate(Evaluator &evaluator, ClosureExpr *closure) const;

public:
  bool isCached() const { return true; }
};

using ProtocolConformanceLookupResult = SmallVector<ProtocolConformance *, 2>;
void simple_display(llvm::raw_ostream &out, ConformanceLookupKind kind);

/// Lookup and expand all conformances in the given context.
///
/// This request specifically accomodates algorithms for retrieving all
/// conformances in the primary, even those that are unstated in source but
/// are implied by other conformances, inherited from other types, or synthesized
/// by the compiler. A simple case of this is the following:
///
/// \code
/// protocol P {}
/// protocol Q : P {}
/// extension T : Q {}
/// \endcode
///
/// Here, a conformance to \c Q has been stated, but a conformance to \c P
/// must also be reported so it can be checked as well.
class LookupAllConformancesInContextRequest
    : public SimpleRequest<LookupAllConformancesInContextRequest,
                           ProtocolConformanceLookupResult(const DeclContext *),
                           RequestFlags::Uncached |
                               RequestFlags::DependencySink |
                               RequestFlags::DependencySource> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  ProtocolConformanceLookupResult
  evaluate(Evaluator &evaluator, const DeclContext *DC) const;

public:
  // Incremental dependencies
  evaluator::DependencySource
  readDependencySource(const evaluator::DependencyCollector &eval) const;
  void writeDependencySink(evaluator::DependencyCollector &tracker,
                           ProtocolConformanceLookupResult r) const;
};

class CheckRedeclarationRequest
    : public SimpleRequest<
          CheckRedeclarationRequest, evaluator::SideEffect(ValueDecl *),
          RequestFlags::SeparatelyCached | RequestFlags::DependencySource |
              RequestFlags::DependencySink> {
public:
  using SimpleRequest::SimpleRequest;

 private:
  friend SimpleRequest;

   // Evaluation.
  evaluator::SideEffect
  evaluate(Evaluator &evaluator, ValueDecl *VD) const;

 public:
  // Separate caching.
  bool isCached() const { return true; }
  Optional<evaluator::SideEffect> getCachedResult() const;
  void cacheResult(evaluator::SideEffect) const;

public:
  evaluator::DependencySource
  readDependencySource(const evaluator::DependencyCollector &eval) const;
  void writeDependencySink(evaluator::DependencyCollector &tracker,
                           evaluator::SideEffect) const;
};

class ResolveTypeEraserTypeRequest
    : public SimpleRequest<ResolveTypeEraserTypeRequest,
                           Type (ProtocolDecl *, TypeEraserAttr *),
                           RequestFlags::SeparatelyCached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  Type evaluate(Evaluator &evaluator, ProtocolDecl *PD,
                TypeEraserAttr *attr) const;

public:
  // Separate caching.
  bool isCached() const { return true; }
  Optional<Type> getCachedResult() const;
  void cacheResult(Type value) const;
};

/// Determines whether this is a "simple" didSet i.e one that either does not
/// use the implicit oldValue parameter in the body or does not take an explicit
/// parameter (ex: 'didSet(oldValue)').
class SimpleDidSetRequest
    : public SimpleRequest<SimpleDidSetRequest, bool(AccessorDecl *),
                           RequestFlags::Cached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  // Evaluation.
  bool evaluate(Evaluator &evaluator, AccessorDecl *decl) const;

public:
  bool isCached() const {
    return std::get<0>(getStorage())->getAccessorKind() == AccessorKind::DidSet;
  }
};

/// A module which has been implicitly imported.
struct ImplicitImport {
  using ImportOptions = SourceFile::ImportOptions;

  ModuleDecl *Module;
  ImportOptions Options;

  ImplicitImport(ModuleDecl *module, ImportOptions opts = {})
      : Module(module), Options(opts) {}

  friend bool operator==(const ImplicitImport &lhs,
                         const ImplicitImport &rhs) {
    return lhs.Module == rhs.Module &&
           lhs.Options.toRaw() == rhs.Options.toRaw();
  }
};

void simple_display(llvm::raw_ostream &out, const ImplicitImport &import);

/// Computes the loaded modules that should be implicitly imported by each file
/// of a given module.
class ModuleImplicitImportsRequest
    : public SimpleRequest<ModuleImplicitImportsRequest,
                           ArrayRef<ImplicitImport>(ModuleDecl *),
                           RequestFlags::Cached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  ArrayRef<ImplicitImport>
  evaluate(Evaluator &evaluator, ModuleDecl *module) const;

public:
  // Cached.
  bool isCached() const { return true; }
};

/// Checks whether a file performs an implementation-only import.
class HasImplementationOnlyImportsRequest
    : public SimpleRequest<HasImplementationOnlyImportsRequest,
                           bool(SourceFile *), RequestFlags::Cached> {
public:
  using SimpleRequest::SimpleRequest;

private:
  friend SimpleRequest;

  bool evaluate(Evaluator &evaluator, SourceFile *SF) const;

public:
  // Cached.
  bool isCached() const { return true; }
};

class ResolveTypeRequest
    : public SimpleRequest<ResolveTypeRequest,
                           Type(TypeResolution *, TypeRepr *),
                           RequestFlags::Uncached> {
public:
  using SimpleRequest::SimpleRequest;

public:
  // Cycle handling.
  void noteCycleStep(DiagnosticEngine &diags) const;

private:
  friend SimpleRequest;

  // Evaluation.
  Type evaluate(Evaluator &evaluator, TypeResolution *resolution,
                TypeRepr *repr) const;
};

void simple_display(llvm::raw_ostream &out, const TypeResolution *resolution);
SourceLoc extractNearestSourceLoc(const TypeRepr *repr);

// Allow AnyValue to compare two Type values, even though Type doesn't
// support ==.
template<>
inline bool AnyValue::Holder<Type>::equals(const HolderBase &other) const {
  assert(typeID == other.typeID && "Caller should match type IDs");
  return value.getPointer() ==
      static_cast<const Holder<Type> &>(other).value.getPointer();
}

// Allow AnyValue to compare two GenericSignature values.
template <>
inline bool
AnyValue::Holder<GenericSignature>::equals(const HolderBase &other) const {
  assert(typeID == other.typeID && "Caller should match type IDs");
  return value.getPointer() ==
         static_cast<const Holder<GenericSignature> &>(other)
             .value.getPointer();
}

void simple_display(llvm::raw_ostream &out, Type value);
void simple_display(llvm::raw_ostream &out, const TypeRepr *TyR);
void simple_display(llvm::raw_ostream &out, ImplicitMemberAction action);
void simple_display(llvm::raw_ostream &out, FunctionBuilderBodyPreCheck pck);

#define SWIFT_TYPEID_ZONE TypeChecker
#define SWIFT_TYPEID_HEADER "swift/AST/TypeCheckerTypeIDZone.def"
#include "swift/Basic/DefineTypeIDZone.h"
#undef SWIFT_TYPEID_ZONE
#undef SWIFT_TYPEID_HEADER

// Set up reporting of evaluated requests.
#define SWIFT_REQUEST(Zone, RequestType, Sig, Caching, LocOptions)             \
  template<>                                                                   \
  inline void reportEvaluatedRequest(UnifiedStatsReporter &stats,              \
                              const RequestType &request) {                    \
    ++stats.getFrontendCounters().RequestType;                                 \
  }
#include "swift/AST/TypeCheckerTypeIDZone.def"
#undef SWIFT_REQUEST

} // end namespace swift

#endif // SWIFT_TYPE_CHECK_REQUESTS_H
