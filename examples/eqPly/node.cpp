
/* Copyright (c) 2006-2011, Stefan Eilemann <eile@equalizergraphics.com> 
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * - Neither the name of Eyescale Software GmbH nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "node.h"

#include "config.h"
#include "error.h"

class Distributable : public co::Serializable
{
public:
    Distributable(eqPly::Node* parent);
	virtual void notifyNewHeadVersion(const eq::uint128_t& version);
	//the version. we will need this to avoid deadlock (for reasons I don't understand).
	eq::uint128_t v;
protected:
private:
	eqPly::Node* node;
};

//MVP: This thread simulates a worker thread. It syncs the Distributable and does some work.
class Worker : public co::base::Thread
{
public:
	Distributable* slave;

    virtual void run()
        {
            //MVP: Uncomment this line to prevent deadlocking.
            //if(slave->v != co::VERSION_FIRST)
            slave->sync();
            //do some work
            co::base::sleep(500);
        }
};

Distributable::Distributable(eqPly::Node* parent)
{
	node = parent;
}
void Distributable::notifyNewHeadVersion(const eq::uint128_t& version)
{
	v = version;
    Worker* worker = new Worker;
    worker->slave = this;
    worker->run();
    // memleak, but who cares here?
}
namespace eqPly
{
	//MVP: This thread simulates the client UI thread. We add a new Distributable every quarter of a second and map it.
class PeriodicMap : public co::base::Thread
{
public:
    eqPly::Node* node;
    virtual void run()
        {
            eq::Config* config = node->getConfig();
            unsigned iter = 0;
            while(true)
            {
                co::base::sleep(250);
                ++iter;
                EQINFO << "Iteration " << iter << std::endl;

                Distributable* master_thing = new Distributable(node);
		
                EQINFO << "Registering master..." << std::endl;
                config->registerObject(master_thing);
                EQINFO << "Master registered." << std::endl;

                Distributable* slave_thing = new Distributable(node);

                EQINFO << "Mapping slave..." << std::endl;
                bool bMapped = config->mapObject( slave_thing,
                                                  master_thing->getID( ));
                if(bMapped)
                    EQINFO << "Slave mapped" << std::endl;
                else
                    EQINFO << "Mapping failed" << std::endl;
            }
        }
};

bool Node::configInit( const eq::uint128_t& initID )
{
    PeriodicMap* mapper = new PeriodicMap;
    mapper->node = this;
    mapper->run();

    if( !eq::Node::configInit( initID ))
        return false;

    // All render data is static or multi-buffered, we can run asynchronously
    if( getIAttribute( IATTR_THREAD_MODEL ) == eq::UNDEFINED )
        setIAttribute( IATTR_THREAD_MODEL, eq::ASYNC );

    Config* config = static_cast< Config* >( getConfig( ));
    if( !config->mapData( initID ))
    {
        setError( ERROR_EQPLY_MAPOBJECT_FAILED );
        return false;
    }
    return true;
}

bool Node::configExit()
{
    Config* config = static_cast< Config* >( getConfig( ));
    config->unmapData();
    
    return eq::Node::configExit();
}

}
