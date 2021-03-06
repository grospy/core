/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * This file incorporates work covered by the following license notice:
 *
 *   Licensed to the Apache Software Foundation (ASF) under one or more
 *   contributor license agreements. See the NOTICE file distributed
 *   with this work for additional information regarding copyright
 *   ownership. The ASF licenses this file to you under the Apache
 *   License, Version 2.0 (the "License"); you may not use this file
 *   except in compliance with the License. You may obtain a copy of
 *   the License at http://www.apache.org/licenses/LICENSE-2.0 .
 */

#include <com/sun/star/accessibility/AccessibleRole.hpp>
#include <com/sun/star/accessibility/AccessibleEventId.hpp>
#include <unotools/accessiblestatesethelper.hxx>
#include <com/sun/star/accessibility/AccessibleStateType.hpp>
#include <com/sun/star/beans/PropertyChangeEvent.hpp>
#include <com/sun/star/awt/XWindow.hpp>
#include <cppuhelper/supportsservice.hxx>
#include <cppuhelper/typeprovider.hxx>
#include <toolkit/helper/convert.hxx>
#include <vcl/svapp.hxx>
#include <vcl/settings.hxx>
#include <osl/mutex.hxx>
#include <rtl/uuid.h>
#include <tools/debug.hxx>
#include <tools/gen.hxx>

#include <svx/strings.hrc>
#include <svx/dlgctrl.hxx>

#include <svx/dialmgr.hxx>
#include <comphelper/accessibleeventnotifier.hxx>

#include <unotools/accessiblerelationsethelper.hxx>

#include <svxpixelctlaccessiblecontext.hxx>
#include <com/sun/star/accessibility/AccessibleRelationType.hpp>

using namespace ::cppu;
using namespace ::osl;
using namespace ::com::sun::star;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::accessibility;

SvxPixelCtlAccessible::SvxPixelCtlAccessible( SvxPixelCtl& rControl) :
    SvxPixelCtlAccessible_BASE(m_aMutex),
    mrPixelCtl(rControl),
    mnClientId(0)
{
}

SvxPixelCtlAccessible::~SvxPixelCtlAccessible()
{
    if( IsAlive() )
    {
        osl_atomic_increment( &m_refCount );
        dispose();      // set mpRepr = NULL & release all children
    }
}
uno::Reference< XAccessibleContext > SvxPixelCtlAccessible::getAccessibleContext(  )
{
    return this;
}

sal_Int32 SvxPixelCtlAccessible::getAccessibleChildCount(  )
{
    ::osl::MutexGuard   aGuard( m_aMutex );
    return SvxPixelCtl::GetSquares();
}
uno::Reference< XAccessible > SvxPixelCtlAccessible::getAccessibleChild( sal_Int32 i )
{
    ::osl::MutexGuard   aGuard( m_aMutex );
    if ( i < 0 || i >= getAccessibleChildCount())
        throw lang::IndexOutOfBoundsException();
    return CreateChild(i, mrPixelCtl.IndexToPoint(i));
}

uno::Reference< XAccessible > SvxPixelCtlAccessible::getAccessibleParent(  )
{
    ::osl::MutexGuard   aGuard( m_aMutex );
    vcl::Window *pTabPage = getNonLayoutParent(&mrPixelCtl);
    if (!pTabPage || WindowType::TABPAGE != pTabPage->GetType())
        return uno::Reference< XAccessible >();
    else
        return pTabPage->GetAccessible();
}

sal_Int32 SvxPixelCtlAccessible::getAccessibleIndexInParent(  )
{
    ::osl::MutexGuard   aGuard( m_aMutex );
    sal_uInt16 nIdx = 0;
    vcl::Window *pTabPage = getNonLayoutParent(&mrPixelCtl);
    if (!pTabPage || WindowType::TABPAGE != pTabPage->GetType())
        return -1;
    sal_uInt16 nChildren = pTabPage->GetChildCount();
    for(nIdx = 0; nIdx < nChildren; nIdx++)
        if(pTabPage->GetChild( nIdx ) == &mrPixelCtl)
            break;
    return nIdx;
}

sal_Int16 SvxPixelCtlAccessible::getAccessibleRole(  )
{
    return AccessibleRole::LIST;
}

OUString SvxPixelCtlAccessible::getAccessibleDescription(  )
{

    ::osl::MutexGuard   aGuard( m_aMutex );
    return mrPixelCtl.GetAccessibleDescription();
}

OUString SvxPixelCtlAccessible::getAccessibleName(  )
{

    ::osl::MutexGuard   aGuard( m_aMutex );
    return mrPixelCtl.GetAccessibleName();
}

uno::Reference< XAccessibleRelationSet > SvxPixelCtlAccessible::getAccessibleRelationSet(  )
{
    ::osl::MutexGuard   aGuard( m_aMutex );
    utl::AccessibleRelationSetHelper* rRelationSet = new utl::AccessibleRelationSetHelper;
    uno::Reference< css::accessibility::XAccessibleRelationSet > rSet = rRelationSet;
    vcl::Window *pLabeledBy = mrPixelCtl.GetAccessibleRelationLabeledBy();
    if ( pLabeledBy && pLabeledBy != &mrPixelCtl )
    {
        uno::Sequence< uno::Reference< uno::XInterface > > aSequence { pLabeledBy->GetAccessible() };
        rRelationSet->AddRelation( css::accessibility::AccessibleRelation( css::accessibility::AccessibleRelationType::LABELED_BY, aSequence ) );
    }

    vcl::Window* pMemberOf = mrPixelCtl.GetAccessibleRelationMemberOf();
    if ( pMemberOf && pMemberOf != &mrPixelCtl )
    {
        uno::Sequence< uno::Reference< uno::XInterface > > aSequence { pMemberOf->GetAccessible() };
        rRelationSet->AddRelation( css::accessibility::AccessibleRelation( css::accessibility::AccessibleRelationType::MEMBER_OF, aSequence ) );
    }
    return rSet;
}


uno::Reference< XAccessibleStateSet > SvxPixelCtlAccessible::getAccessibleStateSet(  )
{
    ::osl::MutexGuard   aGuard( m_aMutex );
    utl::AccessibleStateSetHelper* pStateSetHelper = new utl::AccessibleStateSetHelper;
    uno::Reference< XAccessibleStateSet > xRet = pStateSetHelper;

    const sal_Int16 aStandardStates[] =
    {
        AccessibleStateType::FOCUSABLE,
        AccessibleStateType::SELECTABLE,
        AccessibleStateType::SHOWING,
        AccessibleStateType::VISIBLE,
        AccessibleStateType::OPAQUE,
        0
    };

    sal_Int16 nState = 0;
    while(aStandardStates[nState])
    {
        pStateSetHelper->AddState(aStandardStates[nState++]);
    }
    if(mrPixelCtl.IsEnabled())
        pStateSetHelper->AddState(AccessibleStateType::ENABLED);
    if(mrPixelCtl.HasFocus())
        pStateSetHelper->AddState(AccessibleStateType::FOCUSED);
    pStateSetHelper->AddState(AccessibleStateType::MANAGES_DESCENDANTS);

    return xRet;
}


css::lang::Locale SvxPixelCtlAccessible::getLocale(  )
{
    ::osl::MutexGuard   aGuard( m_aMutex );
    if( getAccessibleParent().is() )
    {
        uno::Reference< XAccessibleContext >        xParentContext( getAccessibleParent()->getAccessibleContext() );
        if( xParentContext.is() )
            return xParentContext->getLocale();
    }

    //  No locale and no parent.  Therefore throw exception to indicate this
    //  cluelessness.
    throw IllegalAccessibleComponentStateException();
}


sal_Bool SvxPixelCtlAccessible::containsPoint( const awt::Point& aPt )
{
    ::osl::MutexGuard   aGuard( m_aMutex );
    Point aPoint(aPt.X, aPt.Y);
    return (aPoint.X() >= 0)
        && (aPoint.X() < mrPixelCtl.GetSizePixel().getWidth())
        && (aPoint.Y() >= 0)
        && (aPoint.Y() < mrPixelCtl.GetSizePixel().getHeight());
}
uno::Reference<XAccessible > SAL_CALL SvxPixelCtlAccessible::getAccessibleAtPoint (
        const awt::Point& aPoint)
{
    ::osl::MutexGuard   aGuard( m_aMutex );
    if( !IsAlive() )
        throw lang::DisposedException();

    Point childPoint;
    childPoint.setX( aPoint.X );
    childPoint.setY( aPoint.Y );

    Point pt= mrPixelCtl.PixelToLogic(childPoint);
    long nIndex = mrPixelCtl.PointToIndex(pt);
    return CreateChild(nIndex, mrPixelCtl.IndexToPoint(nIndex));
}

awt::Rectangle SvxPixelCtlAccessible::getBounds(  )
{
    ::osl::MutexGuard   aGuard( m_aMutex );
    awt::Rectangle aRet;
    Size aSz = mrPixelCtl.GetSizePixel();
    Point aPos = mrPixelCtl.GetPosPixel();
    aRet.X = aPos.X();
    aRet.Y = aPos.Y();
    aRet.Width = aSz.Width();
    aRet.Height = aSz.Height();
    return aRet;
}

awt::Point SvxPixelCtlAccessible::getLocation(  )
{
    ::osl::MutexGuard   aGuard( m_aMutex );
    Point aPos;
    aPos = mrPixelCtl.GetPosPixel();
    awt::Point aRet(aPos.X(), aPos.Y());
    return aRet;
}

awt::Point SvxPixelCtlAccessible::getLocationOnScreen(  )
{
    ::osl::MutexGuard   aGuard( m_aMutex );
    tools::Rectangle rect = mrPixelCtl.GetWindowExtentsRelative(nullptr);
    return awt::Point(rect.Left(),rect.Top() );
}

awt::Size SvxPixelCtlAccessible::getSize(  )
{
    ::osl::MutexGuard   aGuard( m_aMutex );
    Size aSz = mrPixelCtl.GetSizePixel();
    return awt::Size(aSz.Width(),aSz.Height());
}
void SvxPixelCtlAccessible::grabFocus(  )
{
    ::osl::MutexGuard   aGuard( m_aMutex );
    mrPixelCtl.GrabFocus();
}

sal_Int32 SvxPixelCtlAccessible::getForeground(  )
{
    ::osl::MutexGuard   aGuard( m_aMutex );
    return sal_Int32(mrPixelCtl.GetControlForeground());
}

sal_Int32 SvxPixelCtlAccessible::getBackground(  )
{
    ::osl::MutexGuard   aGuard( m_aMutex );
    return sal_Int32(mrPixelCtl.GetControlBackground());
}

OUString SvxPixelCtlAccessible::getImplementationName(  )
{
    return OUString("SvxPixelCtlAccessible");
}

sal_Bool SvxPixelCtlAccessible::supportsService( const OUString& rServiceName )
{
    return cppu::supportsService( this, rServiceName );
}

uno::Sequence< OUString > SvxPixelCtlAccessible::getSupportedServiceNames(  )
{
    uno::Sequence< OUString > aRet(2);
    OUString* pArray = aRet.getArray();
    pArray[0] = "Accessible";
    pArray[1] = "AccessibleContext";
    pArray[2] = "AccessibleComponent";
    return aRet;
}


// XAccessibleSelection

void SAL_CALL SvxPixelCtlAccessible::selectAccessibleChild( sal_Int32 nChildIndex )
{
    ::osl::MutexGuard   aGuard( m_aMutex );

    if ( nChildIndex < 0 || nChildIndex >= getAccessibleChildCount())
        throw lang::IndexOutOfBoundsException();

    long nIndex = mrPixelCtl.ShowPosition(mrPixelCtl.IndexToPoint(nChildIndex));
    NotifyChild(nIndex,true,false);
}

sal_Bool SAL_CALL SvxPixelCtlAccessible::isAccessibleChildSelected( sal_Int32 nChildIndex )
{
    ::osl::MutexGuard   aGuard( m_aMutex );

    return mrPixelCtl.GetFocusPosIndex() == nChildIndex;
}

void SAL_CALL SvxPixelCtlAccessible::clearAccessibleSelection(  )
{
}

void SAL_CALL SvxPixelCtlAccessible::selectAllAccessibleChildren(  )
{
}

sal_Int32 SAL_CALL SvxPixelCtlAccessible::getSelectedAccessibleChildCount(  )
{
    return 1;
}

uno::Reference< XAccessible > SAL_CALL SvxPixelCtlAccessible::getSelectedAccessibleChild( sal_Int32 nSelectedChildIndex )
{
    ::osl::MutexGuard   aGuard( m_aMutex );

    if ( nSelectedChildIndex >= 1)
        throw lang::IndexOutOfBoundsException();

    uno::Reference< XAccessible > xChild;
    if(m_xCurChild.is())
    {
        xChild = m_xCurChild;
    }
    return xChild;
}

void SAL_CALL SvxPixelCtlAccessible::deselectAccessibleChild( sal_Int32 )
{
}

//XAccessibleEventBroadcaster
void SAL_CALL SvxPixelCtlAccessible::addAccessibleEventListener( const uno::Reference< XAccessibleEventListener >& xListener )
{
    if (xListener.is())
    {
        ::osl::MutexGuard   aGuard( m_aMutex );
        if (!mnClientId)
            mnClientId = comphelper::AccessibleEventNotifier::registerClient( );
        comphelper::AccessibleEventNotifier::addEventListener( mnClientId, xListener );
    }
}

void SAL_CALL SvxPixelCtlAccessible::removeAccessibleEventListener( const uno::Reference< XAccessibleEventListener >& xListener )
{
    if (xListener.is())
    {
        ::osl::MutexGuard   aGuard( m_aMutex );
        if (!mnClientId)
            return;
        sal_Int32 nListenerCount = comphelper::AccessibleEventNotifier::removeEventListener( mnClientId, xListener );
        if ( !nListenerCount )
        {
            comphelper::AccessibleEventNotifier::revokeClient( mnClientId );
            mnClientId = 0;
        }
    }
}

//Solution:Add the event handling method
void SvxPixelCtlAccessible::FireAccessibleEvent (short nEventId, const css::uno::Any& rOld, const css::uno::Any& rNew)
{
    const uno::Reference< XInterface >  xSource( *this );
    if (mnClientId)
        comphelper::AccessibleEventNotifier::addEvent( mnClientId, AccessibleEventObject( xSource, nEventId, rNew,rOld ) );
}

void SAL_CALL SvxPixelCtlAccessible::disposing()
{
    if( !rBHelper.bDisposed )
    {
        ::osl::MutexGuard   aGuard( m_aMutex );
        if ( mnClientId )
        {
            comphelper::AccessibleEventNotifier::revokeClientNotifyDisposing( mnClientId, *this );
            mnClientId =  0;
        }
    }
}

void SvxPixelCtlAccessible::NotifyChild(long nIndex,bool bSelect ,bool bCheck)
{
    DBG_ASSERT( !(!bSelect && !bCheck),"" );//non is false

    SvxPixelCtlAccessibleChild *pChild= nullptr;

    if (m_xCurChild.is())
    {
        pChild= static_cast<SvxPixelCtlAccessibleChild*>(m_xCurChild.get());
        DBG_ASSERT(pChild,"Child Must be Valid");
        if (pChild->getAccessibleIndexInParent() == nIndex )
        {
            if (bSelect)
            {
                pChild->SelectChild(true);
            }
            if (bCheck)
            {
                pChild->ChangePixelColorOrBG( mrPixelCtl.GetBitmapPixel(sal_uInt16(nIndex)) != 0);
                pChild->CheckChild();
            }
            return ;
        }
    }
    uno::Reference <XAccessible> xNewChild =CreateChild(nIndex, mrPixelCtl.IndexToPoint(nIndex));
    SvxPixelCtlAccessibleChild *pNewChild= static_cast<SvxPixelCtlAccessibleChild*>(xNewChild.get());
    DBG_ASSERT(pNewChild,"Child Must be Valid");

    Any aNewValue,aOldValue;
    aNewValue<<= xNewChild;
    FireAccessibleEvent(    AccessibleEventId::ACTIVE_DESCENDANT_CHANGED,
                            aOldValue,
                            aNewValue );

    if (bSelect)
    {
        if (pChild)
        {
            pChild->SelectChild(false);
        }
        pNewChild->SelectChild(true);
    }
    if (bCheck)
    {
        pNewChild->CheckChild();
    }
    m_xCurChild= xNewChild;


}

uno::Reference<XAccessible> SvxPixelCtlAccessible::CreateChild (long nIndex,Point mPoint)
{
    bool bPixelColorOrBG = mrPixelCtl.GetBitmapPixel(sal_uInt16(nIndex)) != 0;
    Size size(mrPixelCtl.GetWidth() / SvxPixelCtl::GetLineCount(), mrPixelCtl.GetHeight() / SvxPixelCtl::GetLineCount());
    uno::Reference<XAccessible> xChild;
    xChild = new SvxPixelCtlAccessibleChild(mrPixelCtl,
                bPixelColorOrBG,
                tools::Rectangle(mPoint,size),
                this,
                nIndex);

    return xChild;
}


void SvxPixelCtlAccessible::LoseFocus()
{
    m_xCurChild.clear();
}

void SvxPixelCtlAccessibleChild::CheckChild()
{
    Any aChecked;
    aChecked <<= AccessibleStateType::CHECKED;

    if (m_bPixelColorOrBG)//Current Child State
    {
        FireAccessibleEvent(    AccessibleEventId::STATE_CHANGED,
                                Any(),
                                aChecked);
    }
    else
    {
        FireAccessibleEvent(    AccessibleEventId::STATE_CHANGED,
                                aChecked,
                                Any() );
    }
}

void SvxPixelCtlAccessibleChild::SelectChild( bool bSelect)
{
    Any aSelected;
    aSelected <<= AccessibleStateType::SELECTED;

    if (bSelect)
    {
        FireAccessibleEvent(    AccessibleEventId::STATE_CHANGED,
                                Any(),
                                aSelected);
    }
    else
    {
        FireAccessibleEvent(    AccessibleEventId::STATE_CHANGED,
                                aSelected,
                                Any());
    }
}
void SvxPixelCtlAccessibleChild::FireAccessibleEvent (
    short nEventId,
    const css::uno::Any& rOld,
    const css::uno::Any& rNew)
{
    const uno::Reference< XInterface >  xSource( *this );
    if (mnClientId)
        comphelper::AccessibleEventNotifier::addEvent( mnClientId, AccessibleEventObject( xSource, nEventId, rNew,rOld ) );
}

SvxPixelCtlAccessibleChild::SvxPixelCtlAccessibleChild(
    SvxPixelCtl& rWindow,
    bool bPixelColorOrBG,
    const tools::Rectangle& rBoundingBox,
    const uno::Reference<XAccessible>&  rxParent,
    long nIndexInParent ) :
    SvxPixelCtlAccessibleChild_BASE( m_aMutex ),
    mrParentWindow( rWindow ),
    mxParent(rxParent),
    m_bPixelColorOrBG(bPixelColorOrBG),
    maBoundingBox( rBoundingBox ),
    mnIndexInParent( nIndexInParent ),
    mnClientId( 0 )
{
}


SvxPixelCtlAccessibleChild::~SvxPixelCtlAccessibleChild()
{
    if( IsAlive() )
    {
        osl_atomic_increment( &m_refCount );
        dispose();      // set mpRepr = NULL & release all children
    }
}

// XAccessible
uno::Reference< XAccessibleContext> SAL_CALL SvxPixelCtlAccessibleChild::getAccessibleContext()
{
    return this;
}

// XAccessibleComponent
sal_Bool SAL_CALL SvxPixelCtlAccessibleChild::containsPoint( const awt::Point& rPoint )
{
    // no guard -> done in getBounds()
//  return GetBoundingBox().IsInside( VCLPoint( rPoint ) );
    return tools::Rectangle( Point( 0, 0 ), GetBoundingBox().GetSize() ).IsInside( VCLPoint( rPoint ) );
}

uno::Reference< XAccessible > SAL_CALL SvxPixelCtlAccessibleChild::getAccessibleAtPoint( const awt::Point& )
{
    return uno::Reference< XAccessible >();
}

awt::Rectangle SAL_CALL SvxPixelCtlAccessibleChild::getBounds()
{
    // no guard -> done in getBoundingBox()
    //Modified by lq, 09/26
    //return AWTRectangle( GetBoundingBox() );
    awt::Rectangle rect = AWTRectangle( GetBoundingBox() );
    rect.X = rect.X + mrParentWindow.GetClientWindowExtentsRelative().Left()-mrParentWindow.GetWindowExtentsRelative(nullptr).Left();
    rect.Y = rect.Y + mrParentWindow.GetClientWindowExtentsRelative().Top()-mrParentWindow.GetWindowExtentsRelative(nullptr).Top();
    return rect;
    // End
}

awt::Point SAL_CALL SvxPixelCtlAccessibleChild::getLocation()
{
    // no guard -> done in getBoundingBox()
    return AWTPoint( GetBoundingBox().TopLeft() );
}

awt::Point SAL_CALL SvxPixelCtlAccessibleChild::getLocationOnScreen()
{
    // no guard -> done in getBoundingBoxOnScreen()
    return AWTPoint( GetBoundingBoxOnScreen().TopLeft() );
}

awt::Size SAL_CALL SvxPixelCtlAccessibleChild::getSize()
{
    // no guard -> done in getBoundingBox()
    return AWTSize( GetBoundingBox().GetSize() );
}

void SAL_CALL SvxPixelCtlAccessibleChild::grabFocus()
{
}

sal_Int32 SvxPixelCtlAccessibleChild::getForeground(  )
{
    ::osl::MutexGuard   aGuard( m_aMutex );
    ThrowExceptionIfNotAlive();
    return sal_Int32(mrParentWindow.GetControlForeground());
}
sal_Int32 SvxPixelCtlAccessibleChild::getBackground(  )
{
    ::osl::MutexGuard   aGuard( m_aMutex );

    ThrowExceptionIfNotAlive();
    return sal_Int32(mrParentWindow.GetControlBackground());
}

// XAccessibleContext
sal_Int32 SAL_CALL SvxPixelCtlAccessibleChild::getAccessibleChildCount()
{
    return 0;
}

uno::Reference< XAccessible > SAL_CALL SvxPixelCtlAccessibleChild::getAccessibleChild( sal_Int32 )
{
    throw lang::IndexOutOfBoundsException();
}

uno::Reference< XAccessible > SAL_CALL SvxPixelCtlAccessibleChild::getAccessibleParent()
{
    return mxParent;
}

sal_Int32 SAL_CALL SvxPixelCtlAccessibleChild::getAccessibleIndexInParent()
{
   return mnIndexInParent;
}

sal_Int16 SAL_CALL SvxPixelCtlAccessibleChild::getAccessibleRole()
{
    return AccessibleRole::CHECK_BOX;
}

OUString SAL_CALL SvxPixelCtlAccessibleChild::getAccessibleDescription()
{
    ::osl::MutexGuard   aGuard( m_aMutex );

    return  GetName();
}

OUString SAL_CALL SvxPixelCtlAccessibleChild::getAccessibleName()
{
    ::osl::MutexGuard   aGuard( m_aMutex );
    return  GetName();
}

/** Return empty uno::Reference to indicate that the relation set is not
    supported.
*/
uno::Reference<XAccessibleRelationSet> SAL_CALL SvxPixelCtlAccessibleChild::getAccessibleRelationSet()
{
    return uno::Reference< XAccessibleRelationSet >();
}

uno::Reference< XAccessibleStateSet > SAL_CALL SvxPixelCtlAccessibleChild::getAccessibleStateSet()
{
    ::osl::MutexGuard                       aGuard( m_aMutex );
    utl::AccessibleStateSetHelper*          pStateSetHelper = new utl::AccessibleStateSetHelper;

    if( IsAlive() )
    {

        pStateSetHelper->AddState( AccessibleStateType::TRANSIENT );
        pStateSetHelper->AddState( AccessibleStateType::ENABLED );
        pStateSetHelper->AddState( AccessibleStateType::OPAQUE );
        pStateSetHelper->AddState( AccessibleStateType::SELECTABLE );
        pStateSetHelper->AddState( AccessibleStateType::SHOWING );
        pStateSetHelper->AddState( AccessibleStateType::VISIBLE );

        long nIndex = mrParentWindow.GetFocusPosIndex();
        if ( nIndex == mnIndexInParent)
        {
            pStateSetHelper->AddState( AccessibleStateType::SELECTED );
        }
        if (mrParentWindow.GetBitmapPixel(sal_uInt16(mnIndexInParent)))
        {
            pStateSetHelper->AddState( AccessibleStateType::CHECKED );
        }
    }
    else
        pStateSetHelper->AddState( AccessibleStateType::DEFUNC );

    return pStateSetHelper;
}

lang::Locale SAL_CALL SvxPixelCtlAccessibleChild::getLocale()
{
    ::osl::MutexGuard                       aGuard( m_aMutex );
    if( mxParent.is() )
    {
        uno::Reference< XAccessibleContext >        xParentContext( mxParent->getAccessibleContext() );
        if( xParentContext.is() )
            return xParentContext->getLocale();
    }

    //  No locale and no parent.  Therefore throw exception to indicate this
    //  cluelessness.
    throw IllegalAccessibleComponentStateException();
}

void SAL_CALL SvxPixelCtlAccessibleChild::addAccessibleEventListener( const uno::Reference< XAccessibleEventListener >& xListener )
{
    if (xListener.is())
    {
        ::osl::MutexGuard   aGuard( m_aMutex );
        if (!mnClientId)
            mnClientId = comphelper::AccessibleEventNotifier::registerClient( );
        comphelper::AccessibleEventNotifier::addEventListener( mnClientId, xListener );
    }
}

void SAL_CALL SvxPixelCtlAccessibleChild::removeAccessibleEventListener( const uno::Reference< XAccessibleEventListener >& xListener )
{
    if (xListener.is())
    {
        ::osl::MutexGuard   aGuard( m_aMutex );

        sal_Int32 nListenerCount = comphelper::AccessibleEventNotifier::removeEventListener( mnClientId, xListener );
        if ( !nListenerCount )
        {
            // no listeners anymore
            // -> revoke ourself. This may lead to the notifier thread dying (if we were the last client),
            // and at least to us not firing any events anymore, in case somebody calls
            // NotifyAccessibleEvent, again
            comphelper::AccessibleEventNotifier::revokeClient( mnClientId );
            mnClientId = 0;
        }
    }
}

// XServiceInfo
OUString SAL_CALL SvxPixelCtlAccessibleChild::getImplementationName()
{
    return OUString( "SvxPixelCtlAccessibleChild" );
}

sal_Bool SAL_CALL SvxPixelCtlAccessibleChild::supportsService( const OUString& rServiceName )
{
    return cppu::supportsService( this, rServiceName );
}

Sequence< OUString > SAL_CALL SvxPixelCtlAccessibleChild::getSupportedServiceNames()
{
    uno::Sequence< OUString > aRet(3);
    OUString* pArray = aRet.getArray();
    pArray[0] = "Accessible";
    pArray[1] = "AccessibleContext";
    pArray[2] = "AccessibleComponent";
    return aRet;
}

void SAL_CALL SvxPixelCtlAccessibleChild::disposing()
{
    if( !rBHelper.bDisposed )
    {
        ::osl::MutexGuard   aGuard( m_aMutex );

        // Send a disposing to all listeners.
        if ( mnClientId )
        {
            comphelper::AccessibleEventNotifier::revokeClientNotifyDisposing( mnClientId, *this );
            mnClientId =  0;
        }

        mxParent.clear();
    }
}

void SvxPixelCtlAccessibleChild::ThrowExceptionIfNotAlive()
{
    if( rBHelper.bDisposed || rBHelper.bInDispose )
        throw lang::DisposedException();
}

tools::Rectangle SvxPixelCtlAccessibleChild::GetBoundingBoxOnScreen()
{
    ::osl::MutexGuard   aGuard( m_aMutex );

    // no ThrowExceptionIfNotAlive() because its done in GetBoundingBox()
    tools::Rectangle           aRect( GetBoundingBox() );

    return tools::Rectangle( mrParentWindow.OutputToAbsoluteScreenPixel( aRect.TopLeft() ), aRect.GetSize() );
}

tools::Rectangle const & SvxPixelCtlAccessibleChild::GetBoundingBox()
{
    // no guard necessary, because no one changes maBoundingBox after creating it
    ThrowExceptionIfNotAlive();

    return maBoundingBox;
}

OUString SvxPixelCtlAccessibleChild::GetName()
{
    sal_Int32 nXIndex = mnIndexInParent % SvxPixelCtl::GetLineCount();
    sal_Int32 nYIndex = mnIndexInParent / SvxPixelCtl::GetLineCount();

    OUString str = "("
                 + OUString::number(nXIndex)
                 + ","
                 + OUString::number(nYIndex)
                 + ")";
    return str;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
