
/* Copyright (c) 2009-2010, Stefan Eilemann <eile@equalizergraphics.com> 
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 2.1 as published
 * by the Free Software Foundation.
 *  
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef EQSERVER_VIEW_H
#define EQSERVER_VIEW_H

#include "types.h"

#include <eq/fabric/view.h>     // base class
#include <eq/fabric/viewport.h> // member

namespace eq
{
namespace server
{
    class Config;
    class Layout;

    /** 
     * Extends the eq::View to implement server-side logic.
     */
    class View : public fabric::View< Layout, View, Observer >
    {
    public:
        EQSERVER_EXPORT View( Layout* parent );

        /** Create a new, deep copy of a view. */
        View( const View& from, Layout* parent );

        virtual ~View();

        /** @name Data Access. */
        //@{
        /** @return the config of this view. */
        Config* getConfig();

        /** @return the config of this view. */
        const Config* getConfig() const;

        /** @return the index path to this view. */
        ViewPath getPath() const;

        /** 
         * Adds a new destination channel to this view.
         * 
         * @param channel the channel.
         */
        void addChannel( Channel* channel );
        
        /** 
         * Removes a destination channel from this view.
         * 
         * @param channel the channel
         * @return <code>true</code> if the channel was removed, 
         *         <code>false</code> otherwise.
         */
        bool removeChannel( Channel* channel );
        
        /** @return the vector of channels. */
        const ChannelVector& getChannels() const{ return _channels; }
        //@}

    protected:
        /** @sa eq::View::deserialize() */
        virtual void deserialize( net::DataIStream& is, 
                                  const uint64_t dirtyBits );

    private:

        /** The list of channels. */
        ChannelVector _channels;

        union // placeholder for binary-compatible changes
        {
            char dummy[32];
        };
    };
}
}
#endif // EQSERVER_VIEW_H
